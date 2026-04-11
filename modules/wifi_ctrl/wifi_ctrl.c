/**
 * @file wifi_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief Wi-Fi STA controller
 * @version 0.1
 * @date 2026-04-03
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 *
 * Scan: esp_wifi_scan_start() with block=true on a worker task, then esp_wifi_scan_get_ap_records().
 * Connect: esp_wifi_set_config(WIFI_IF_STA) + esp_wifi_connect(); credentials in MSG_TYPE_WIFI_CONNECT.
 * STA IPv4: `IP_EVENT_STA_GOT_IP` only enqueues `MSG_TYPE_WIFI_CTRL_GOT_IP`; the worker reads netif/MAC and `MGR_Send`s.
 *
 * See ESP-IDF Programming Guide: Wi-Fi Driver > Scan, and Wi-Fi > Station General Scenario.
 */
#include <string.h>

#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "err.h"
#include "lut.h"
#include "mgr_ctrl.h"
#include "msg.h"
#include "wifi_ctrl.h"

/**
 * @name Worker task and queue sizing
 * @{
 */
#define WIFI_TASK_NAME          "wifi-ctrl-task"
#define WIFI_TASK_STACK_SIZE    6144
#define WIFI_TASK_PRIORITY      11

#define WIFI_MSG_MAX            8

/** Max time to wait for a slot to post `MSG_TYPE_DONE` during shutdown (worker must drain the queue). */
#define WIFI_CTRL_SHUTDOWN_QUEUE_TIMEOUT_MS 60000U
/** Max time to wait for the worker to exit after `MSG_TYPE_DONE` is queued. */
#define WIFI_CTRL_SHUTDOWN_SEM_TIMEOUT_MS   10000U
/** @} */

static const char *TAG = "ESP::WIFI";

/**
 * @name File-local module state
 * @brief Static state for STA netif, command queue, and last scan snapshot.
 *
 * | Symbol | Role |
 * |--------|------|
 * | s_wifi_queue | Command queue: scan, connect, disconnect, shutdown (MSG_TYPE_DONE). |
 * | s_wifi_task | FreeRTOS worker; runs blocking `esp_wifi_scan_start()` and connect. |
 * | s_wifi_done_sem | Binary semaphore given when the worker task exits after shutdown. |
 * | s_sta_netif | Default Wi-Fi STA `esp_netif` from `esp_netif_create_default_wifi_sta()`. |
 * | s_ap_records | Buffer filled by `esp_wifi_scan_get_ap_records()` (size `CONFIG_WIFI_CTRL_SCAN_MAX_AP`). |
 * | s_ap_count | Number of valid entries in `s_ap_records` after the last successful scan. |
 * | s_wifi_started | Set after `WifiCtrl_Run()` successfully calls `esp_wifi_start()`. |
 * @{
 */
static QueueHandle_t     s_wifi_queue = NULL;
static TaskHandle_t      s_wifi_task = NULL;
static SemaphoreHandle_t s_wifi_done_sem = NULL;
static esp_netif_t      *s_sta_netif = NULL;

static wifi_ap_record_t s_ap_records[CONFIG_WIFI_CTRL_SCAN_MAX_AP];
static uint16_t         s_ap_count = 0;
static bool             s_wifi_started = false;
/** @} */

static esp_err_t wifictrl_Enqueue(const msg_t *msg);

/**
 * @brief Ensure TCP/IP stack and default event loop exist (idempotent).
 *
 * @return ESP_OK on success or if already initialised; otherwise the first failing init error.
 */
static esp_err_t wifictrl_EnsureNetifAndEvents(void)
{
  esp_err_t err = esp_netif_init();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "esp_netif_init: %s", esp_err_to_name(err));
    return err;
  }
  err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "esp_event_loop_create_default: %s", esp_err_to_name(err));
    return err;
  }
  return ESP_OK;
}

/**
 * @brief Post a Wi-Fi application event to the manager for broadcast (`REG_ALL_CTRL`).
 *
 * @param id Application-level event id (`data_wifi_event_e`).
 *
 * @return Result of `MGR_Send()`.
 */
static esp_err_t wifictrl_SendWifiEvent(data_wifi_event_e id)
{
  msg_t msg = {
    .type = MSG_TYPE_WIFI_EVENT,
    .from = REG_WIFI_CTRL,
    .to = REG_ALL_CTRL,
    .payload.wifi.u.event_id = id,
  };
  return MGR_Send(&msg);
}

/**
 * @brief Default event loop callback for IPv4 address on the STA interface.
 *
 * Keeps `sys_evt` shallow: only enqueues `MSG_TYPE_WIFI_CTRL_GOT_IP` for the worker task, which reads netif + MAC
 * and sends `MSG_TYPE_WIFI_IP` / `MSG_TYPE_WIFI_MAC` via `MGR_Send`.
 *
 * @param arg        Unused.
 * @param event_base Expected `IP_EVENT`.
 * @param event_id   Only `IP_EVENT_STA_GOT_IP` is handled.
 * @param event_data Unused (IP is read on the worker with `esp_netif_get_ip_info`).
 */
static void wifictrl_OnIpEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  (void)arg;
  (void)event_base;
  (void)event_data;

  if (event_id != IP_EVENT_STA_GOT_IP) {
    return;
  }

  msg_t msg = {
    .type = MSG_TYPE_WIFI_CTRL_GOT_IP,
    .from = REG_WIFI_CTRL,
    .to = REG_WIFI_CTRL,
  };

  esp_err_t err = wifictrl_Enqueue(&msg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "enqueue GOT_IP failed: %s", esp_err_to_name(err));
  }
}

/**
 * @brief Read STA IPv4 from `s_sta_netif` and publish to the manager and LCD.
 *
 * Intended to run on the Wi-Fi worker task (larger stack than `sys_evt`).
 *
 * @return ESP_OK if IP was read and forwarded (MAC send failure is logged only); errors from `esp_netif_get_ip_info`.
 */
static esp_err_t wifictrl_PublishStaIpAndMac(void)
{
  if (s_sta_netif == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  esp_netif_ip_info_t ip_info;
  esp_err_t err = esp_netif_get_ip_info(s_sta_netif, &ip_info);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_netif_get_ip_info: %s", esp_err_to_name(err));
    return err;
  }

  msg_t msg = {
    .type = MSG_TYPE_WIFI_IP,
    .from = REG_WIFI_CTRL,
    .to = REG_MGR_CTRL | REG_LCD_CTRL,
  };
  msg.payload.wifi.u.ip_info.ip   = ip_info.ip.addr;
  msg.payload.wifi.u.ip_info.mask = ip_info.netmask.addr;
  msg.payload.wifi.u.ip_info.gw   = ip_info.gw.addr;

  ESP_LOGI(TAG, "WiFi got IP: " IPSTR " mask " IPSTR " gw " IPSTR,
           IP2STR(&ip_info.ip), IP2STR(&ip_info.netmask), IP2STR(&ip_info.gw));

  err = MGR_Send(&msg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "MGR_Send(WIFI_IP) failed: %s", esp_err_to_name(err));
  }

  uint8_t sta_mac[6];
  if (esp_wifi_get_mac(WIFI_IF_STA, sta_mac) == ESP_OK) {
    msg_t mac_msg = {
      .type = MSG_TYPE_WIFI_MAC,
      .from = REG_WIFI_CTRL,
      .to = REG_LCD_CTRL,
    };
    memcpy(mac_msg.payload.wifi.u.mac, sta_mac, sizeof(sta_mac));
    err = MGR_Send(&mac_msg);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "MGR_Send(WIFI_MAC) failed: %s", esp_err_to_name(err));
    }
  }

  return ESP_OK;
}

/**
 * @brief Default event loop callback for `WIFI_EVENT` (STA lifecycle).
 *
 * Maps driver events to `MSG_TYPE_WIFI_EVENT` with `data_wifi_event_e` payloads.
 *
 * @param arg        Unused.
 * @param event_base Expected `WIFI_EVENT`.
 * @param event_id   `WIFI_EVENT_STA_*` values.
 * @param event_data Unused for the handled IDs.
 */
static void wifictrl_OnWifiEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  (void)arg;
  (void)event_base;
  (void)event_data;

  switch (event_id) {
    case WIFI_EVENT_STA_START: {
      (void)wifictrl_SendWifiEvent(DATA_WIFI_EVENT_STA_START);
      break;
    }
    case WIFI_EVENT_STA_STOP: {
      (void)wifictrl_SendWifiEvent(DATA_WIFI_EVENT_STA_STOP);
      break;
    }
    case WIFI_EVENT_STA_CONNECTED: {
      (void)wifictrl_SendWifiEvent(DATA_WIFI_EVENT_CONNECTED);
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
      (void)wifictrl_SendWifiEvent(DATA_WIFI_EVENT_DISCONNECTED);
      break;
    }
    default: {
      break;
    }
  }
}

/**
 * @brief Run a blocking STA scan and publish results to the manager.
 *
 * Clears `s_ap_count`, calls `esp_wifi_scan_start()` with `block=true`, fills `s_ap_records`,
 * then logs each AP (SSID, BSSID, channel, RSSI, auth) and sends `MSG_TYPE_WIFI_SCAN_RESULT`
 * with `ap_count` only. On failure, sends `DATA_WIFI_EVENT_SCAN_FAILED`.
 *
 * @return ESP_OK after a successful scan (notification errors are logged only); driver errors on scan failure.
 */
static esp_err_t wifictrl_DoScan(void)
{
  uint16_t number = CONFIG_WIFI_CTRL_SCAN_MAX_AP;

  wifi_scan_config_t scan_cfg = {
    .ssid       = NULL,
    .bssid      = NULL,
    .channel    = 0,
    .show_hidden = true,
  };
  msg_t msg = {
    .type = MSG_TYPE_WIFI_SCAN_RESULT,
    .from = REG_WIFI_CTRL,
    .to = REG_ALL_CTRL,
  };

  esp_err_t err = ESP_OK;

  s_ap_count = 0;

  err = esp_wifi_scan_start(&scan_cfg, true);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_scan_start: %s", esp_err_to_name(err));
    (void)wifictrl_SendWifiEvent(DATA_WIFI_EVENT_SCAN_FAILED);
    return err;
  }

  err = esp_wifi_scan_get_ap_records(&number, s_ap_records);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_scan_get_ap_records: %s", esp_err_to_name(err));
    (void)wifictrl_SendWifiEvent(DATA_WIFI_EVENT_SCAN_FAILED);
    return err;
  }
  s_ap_count = number;

  ESP_LOGI(TAG, "WiFi scan: %u network(s)", (unsigned)s_ap_count);
  for (uint16_t i = 0; i < s_ap_count; ++i) {
    const wifi_ap_record_t *ap = &s_ap_records[i];
    char ssid_str[sizeof(ap->ssid) + 1U];
    size_t sl = strnlen((const char *)ap->ssid, sizeof(ap->ssid));
    memcpy(ssid_str, ap->ssid, sl);
    ssid_str[sl] = '\0';
    ESP_LOGI(TAG,
             "  [%u] ssid=\"%s\" bssid=%02x:%02x:%02x:%02x:%02x:%02x ch=%u rssi=%d authmode=%d",
             (unsigned)i, ssid_str, ap->bssid[0], ap->bssid[1], ap->bssid[2], ap->bssid[3], ap->bssid[4],
             ap->bssid[5], (unsigned)ap->primary, (int)ap->rssi, (int)ap->authmode);
  }

  msg.payload.wifi.u.scan.ap_count = s_ap_count;

  err = MGR_Send(&msg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "MGR_Send(SCAN_RESULT) failed: %s", esp_err_to_name(err));
  }
  return ESP_OK;
}

/**
 * @brief Apply STA credentials and start association.
 *
 * @param c Connection parameters (`ssid`, `password`, optional `channel`, `authmode` as `wifi_auth_mode_t`).
 *          SSID and password may be up to 32 and 64 octets respectively (`wifi_sta_config_t`); they need not be
 *          NUL-terminated when using the full length.
 *
 * @return ESP_OK if association was requested; `ESP_ERR_INVALID_ARG` for bad lengths; driver errors otherwise.
 */
static esp_err_t wifictrl_DoConnect(const data_wifi_connect_t *c)
{
  if (c == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  wifi_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));

  size_t ssid_len = strnlen(c->ssid, sizeof(cfg.sta.ssid));
  size_t pass_len = strnlen(c->password, sizeof(cfg.sta.password));
  if (ssid_len == 0U || ssid_len > sizeof(cfg.sta.ssid)) {
    ESP_LOGE(TAG, "Invalid SSID length");
    return ESP_ERR_INVALID_ARG;
  }
  if (pass_len > sizeof(cfg.sta.password)) {
    ESP_LOGE(TAG, "Invalid password length");
    return ESP_ERR_INVALID_ARG;
  }

  memcpy(cfg.sta.ssid, c->ssid, ssid_len);
  memcpy(cfg.sta.password, c->password, pass_len);
  if (c->channel != 0U) {
    cfg.sta.channel = c->channel;
  }
  cfg.sta.threshold.authmode = (wifi_auth_mode_t)c->authmode;

  esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_set_config: %s", esp_err_to_name(err));
    return err;
  }
  err = esp_wifi_connect();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_connect: %s", esp_err_to_name(err));
  }
  return err;
}

/**
 * @brief Dispatch one message from the worker queue.
 *
 * @param msg Incoming command; handles `MSG_TYPE_DONE`, `MSG_TYPE_WIFI_CTRL_GOT_IP`, `MSG_TYPE_WIFI_SCAN_REQ`,
 *            `MSG_TYPE_WIFI_CONNECT`, `MSG_TYPE_WIFI_DISCONNECT`.
 *
 * @return `ESP_TASK_DONE` to stop the worker loop; ESP_OK or an `esp_err_t` for other paths.
 */
static esp_err_t wifictrl_ParseMsg(const msg_t *msg)
{
  esp_err_t result = ESP_OK;

  switch (msg->type) {
    case MSG_TYPE_DONE: {
      result = ESP_TASK_DONE;
      break;
    }
    case MSG_TYPE_WIFI_CTRL_GOT_IP: {
      result = wifictrl_PublishStaIpAndMac();
      break;
    }
    case MSG_TYPE_WIFI_SCAN_REQ: {
      result = wifictrl_DoScan();
      break;
    }
    case MSG_TYPE_WIFI_CONNECT: {
      result = wifictrl_DoConnect(&msg->payload.wifi.u.connect);
      break;
    }
    case MSG_TYPE_WIFI_DISCONNECT: {
      result = esp_wifi_disconnect();
      break;
    }
    default: {
      result = ESP_OK;
      break;
    }
  }
  return result;
}

/**
 * @brief Worker task: dequeue `msg_t` and run `wifictrl_ParseMsg()`.
 *
 * Signals `s_wifi_done_sem` and self-deletes when `MSG_TYPE_DONE` is processed.
 *
 * @param param Unused (NULL).
 */
static void wifictrl_TaskFn(void *param)
{
  (void)param;
  msg_t msg;
  bool loop = true;

  ESP_LOGI(TAG, "worker task started");
  memset(&msg, 0, sizeof(msg));

  while (loop) {
    if (xQueueReceive(s_wifi_queue, &msg, portMAX_DELAY) != pdTRUE) {
      continue;
    }
    esp_err_t r = wifictrl_ParseMsg(&msg);
    if (r == ESP_TASK_DONE) {
      loop = false;
    } else if (r != ESP_OK) {
      ESP_LOGW(TAG, "command handling error: %s", esp_err_to_name(r));
    }
  }

  if (s_wifi_done_sem) {
    xSemaphoreGive(s_wifi_done_sem);
  }
  vTaskDelete(NULL);
}

/**
 * @brief Non-blocking post to `s_wifi_queue` (zero tick timeout).
 *
 * @param msg Pointer to message to copy into the queue.
 *
 * @return ESP_OK on success; `ESP_ERR_INVALID_STATE` if the queue is missing; `ESP_FAIL` if full.
 */
static esp_err_t wifictrl_Enqueue(const msg_t *msg)
{
  if (s_wifi_queue == NULL) {
    return ESP_ERR_INVALID_STATE;
  }
  if (xQueueSend(s_wifi_queue, msg, (TickType_t)0) != pdPASS) {
    ESP_LOGE(TAG, "queue full, type %d", (int)msg->type);
    return ESP_FAIL;
  }
  return ESP_OK;
}

/**
 * @brief Undo partial `WifiCtrl_Init`: task, handlers, Wi-Fi driver, netif, queue, semaphore.
 *
 * Safe when only a subset of those objects exists; clears module statics.
 *
 * @param err Error code to return after cleanup.
 *
 * @return `err` unchanged.
 */
static esp_err_t wifictrl_RollbackInit(esp_err_t err)
{
  if (s_wifi_task) {
    vTaskDelete(s_wifi_task);
    s_wifi_task = NULL;
  }
  (void)esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifictrl_OnIpEvent);
  (void)esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifictrl_OnWifiEvent);
  (void)esp_wifi_deinit();
  if (s_sta_netif) {
    esp_netif_destroy(s_sta_netif);
    s_sta_netif = NULL;
  }
  if (s_wifi_queue) {
    vQueueDelete(s_wifi_queue);
    s_wifi_queue = NULL;
  }
  if (s_wifi_done_sem) {
    vSemaphoreDelete(s_wifi_done_sem);
    s_wifi_done_sem = NULL;
  }
  return err;
}

/**
 * @brief Initialise Wi-Fi STA driver, netif, event handlers, and the worker task.
 *
 * Does not call `esp_wifi_start()`; use `WifiCtrl_Run()` after all controllers are initialised.
 *
 * @return ESP_OK on success; `ESP_ERR_NO_MEM` or driver errors on failure (rolls back partial init).
 */
esp_err_t WifiCtrl_Init(void)
{
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_WIFI_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  result = wifictrl_EnsureNetifAndEvents();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "netif/events: %s", esp_err_to_name(result));
    return result;
  }

  s_wifi_queue = xQueueCreate(WIFI_MSG_MAX, sizeof(msg_t));
  if (s_wifi_queue == NULL) {
    ESP_LOGE(TAG, "queue create failed");
    return wifictrl_RollbackInit(ESP_ERR_NO_MEM);
  }

  s_wifi_done_sem = xSemaphoreCreateBinary();
  if (s_wifi_done_sem == NULL) {
    ESP_LOGE(TAG, "done semaphore create failed");
    return wifictrl_RollbackInit(ESP_ERR_NO_MEM);
  }

  s_sta_netif = esp_netif_create_default_wifi_sta();
  if (s_sta_netif == NULL) {
    ESP_LOGE(TAG, "sta netif create failed");
    return wifictrl_RollbackInit(ESP_FAIL);
  }

  wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
  result = esp_wifi_init(&wcfg);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_init: %s", esp_err_to_name(result));
    return wifictrl_RollbackInit(result);
  }

  result = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifictrl_OnWifiEvent, NULL);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "WIFI_EVENT register: %s", esp_err_to_name(result));
    return wifictrl_RollbackInit(result);
  }
  result = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifictrl_OnIpEvent, NULL);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "IP_EVENT register: %s", esp_err_to_name(result));
    return wifictrl_RollbackInit(result);
  }

  BaseType_t ok = xTaskCreate(wifictrl_TaskFn, WIFI_TASK_NAME, WIFI_TASK_STACK_SIZE, NULL, WIFI_TASK_PRIORITY, &s_wifi_task);
  if (ok != pdPASS) {
    ESP_LOGE(TAG, "xTaskCreate failed");
    return wifictrl_RollbackInit(ESP_FAIL);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Tear down STA: stop Wi-Fi, stop worker, unregister handlers, destroy netif and RTOS objects.
 *
 * @return ESP_OK.
 */
esp_err_t WifiCtrl_Done(void)
{
  ESP_LOGI(TAG, "++%s()", __func__);

  if (s_wifi_started) {
    (void)esp_wifi_stop();
    s_wifi_started = false;
  }

  if (s_wifi_queue != NULL) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_WIFI_CTRL,
      .to = REG_WIFI_CTRL,
    };

    bool worker_stopped =
        (xQueueSend(s_wifi_queue, &msg, pdMS_TO_TICKS(WIFI_CTRL_SHUTDOWN_QUEUE_TIMEOUT_MS)) == pdPASS);
    if (worker_stopped && s_wifi_done_sem != NULL) {
      worker_stopped =
          (xSemaphoreTake(s_wifi_done_sem, pdMS_TO_TICKS(WIFI_CTRL_SHUTDOWN_SEM_TIMEOUT_MS)) == pdTRUE);
    } else if (worker_stopped) {
      worker_stopped = false;
    }

    if (!worker_stopped) {
      ESP_LOGE(TAG, "shutdown: worker did not stop cleanly, deleting task");
      if (s_wifi_task != NULL) {
        vTaskDelete(s_wifi_task);
        vTaskDelay(pdMS_TO_TICKS(1));
      }
    }

    s_wifi_task = NULL;
    vQueueDelete(s_wifi_queue);
    s_wifi_queue = NULL;
  } else {
    s_wifi_task = NULL;
  }

  (void)esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifictrl_OnIpEvent);
  (void)esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifictrl_OnWifiEvent);

  (void)esp_wifi_deinit();

  if (s_sta_netif) {
    esp_netif_destroy(s_sta_netif);
    s_sta_netif = NULL;
  }

  if (s_wifi_done_sem) {
    vSemaphoreDelete(s_wifi_done_sem);
    s_wifi_done_sem = NULL;
  }

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}

/**
 * @brief Start the Wi-Fi driver in STA mode (`esp_wifi_set_mode` + `esp_wifi_start`).
 *
 * @return ESP_OK on success; driver error otherwise.
 */
esp_err_t WifiCtrl_Run(void)
{
  ESP_LOGI(TAG, "++%s()", __func__);
  esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_set_mode: %s", esp_err_to_name(err));
    return err;
  }
  err = esp_wifi_start();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_start: %s", esp_err_to_name(err));
    return err;
  }
  s_wifi_started = true;
  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}

/**
 * @brief Enqueue Wi-Fi commands for the worker task.
 *
 * Accepts `MSG_TYPE_WIFI_SCAN_REQ`, `MSG_TYPE_WIFI_CONNECT`, and `MSG_TYPE_WIFI_DISCONNECT`;
 * other types are ignored (returns ESP_OK). `MSG_TYPE_WIFI_CTRL_GOT_IP` is for internal use only.
 *
 * @param msg Message to queue-copy; must not be NULL.
 *
 * @return ESP_OK if accepted or ignored; `ESP_ERR_INVALID_ARG` if `msg` is NULL; queue errors from `wifictrl_Enqueue`.
 */
esp_err_t WifiCtrl_Send(const msg_t *msg)
{
  if (msg == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  return wifictrl_Enqueue(msg);
}
