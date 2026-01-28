/**
 * @file sys_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief System Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_mac.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

#include "sdkconfig.h"

#include "err.h"
#include "msg.h"
#include "sys_ctrl.h"

#include "err.h"
#include "lut.h"
#include "sys_lut.h"


#define SYS_TASK_NAME           "sys-task"
#define SYS_TASK_STACK_SIZE     4096
#define SYS_TASK_PRIORITY       12

#define SYS_MSG_MAX             10


static const char* TAG = "ESP::SYS";


static QueueHandle_t      sys_msg_queue = NULL;
static TaskHandle_t       sys_task_id = NULL;
static SemaphoreHandle_t  sys_sem_id = NULL;


static data_eth_mac_t     sys_esp_mac = {0};

/**
 * @brief Time synchronization notification callback
 * 
 * @param tv Pointer to a timeval structure containing the synchronized time
 */
static void sysctrl_TimeSyncNotificationCb(struct timeval *tv)
{
  ESP_LOGI(TAG, "[%s] Notification of a time synchronization event", __func__);
  //sntp_sync_time(tv);
}

/**
 * @brief Initialize SNTP
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_InitSNTP(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  config.sync_cb = sysctrl_TimeSyncNotificationCb;  // only if we need the notification function
  esp_netif_sntp_init(&config);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Wait for time to be set via SNTP
 * 
 */
static void sysctrl_WaitTime(void) {
  // wait for time to be set
  int retry = 0;
  const int retry_count = 15;

  ESP_LOGI(TAG, "++%s()", __func__);

  while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
    ESP_LOGD(TAG, "[%s] Waiting for system time to be set... (%d/%d)", __func__, retry, retry_count);
  }

  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Set time zone
 * 
 * @param tz_str Time zone string (e.g., "PST8PDT")
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_setTimeZone(const char* tz_str) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (setenv("TZ", tz_str, 1) == 0) {
    time_t now;
    struct tm tm;
    char buf[64];

    tzset();

    ESP_LOGD(TAG, "tzname[0]=%s tzname[1]=%s",
             tzname[0] ? tzname[0] : "-", tzname[1] ? tzname[1] : "-");

    now = time(NULL);
    localtime_r(&now, &tm);

    strftime(buf, sizeof(buf), "%Z %z", &tm);
    ESP_LOGD(TAG, "Current local zone: %s", buf);

  } else {
    ESP_LOGE(TAG, "setenv(TZ) failed");
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get time zone information
 * 
 */
static void sysctrl_GetTimeZone(void) {
  const char *env_tz = getenv("TZ");

  ESP_LOGI(TAG, "++%s()", __func__);
  if (env_tz) {
    ESP_LOGD(TAG, "TZ env: %s", env_tz);
  } else {
    ESP_LOGW(TAG, "TZ env not set");
  }

  ESP_LOGD(TAG, "tzname[0]=%s tzname[1]=%s",
            tzname[0] ? tzname[0] : "-", tzname[1] ? tzname[1] : "-");

  time_t now = time(NULL);
  struct tm tm;
  localtime_r(&now, &tm);
  char buf[64];
  strftime(buf, sizeof(buf), "%Z %z", &tm); // %Z = zone name, %z = offset (+/-HHMM)
  ESP_LOGD(TAG, "Current local zone: %s", buf);

  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Get current time
 * 
 */
static void sysctrl_GetTime(void) {
  time_t now;
  char strftime_buf[64];
  struct tm timeinfo;

  ESP_LOGI(TAG, "++%s()", __func__);
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGD(TAG, "[%s] The current date/time in is: %s", __func__, strftime_buf);
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Get MAC address
 * 
 * @param mac Pointer to a buffer where the MAC address will be stored
 * @param type The type of MAC address to retrieve
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_GetMacAddress(uint8_t *mac_ptr, esp_mac_type_t type) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s])", __func__, type, GET_ESP_MAC_TYPE_NAME(type));
  if (mac_ptr == NULL) {
    ESP_LOGE(TAG, "MAC address buffer is NULL");
    result = ESP_ERR_INVALID_ARG;
  } else {
    result = esp_read_mac(mac_ptr, type);
    if (result == ESP_OK) {
      ESP_LOGD(TAG, "MAC Address [%s]: %02X:%02X:%02X:%02X:%02X:%02X",
               GET_ESP_MAC_TYPE_NAME(type),
               mac_ptr[0], mac_ptr[1], mac_ptr[2], mac_ptr[3], mac_ptr[4], mac_ptr[5]);
    } else {
      ESP_LOGE(TAG, "Failed to read MAC address, error: %d", result);
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Handle Ethernet events
 * 
 * @param event_id The specific event ID within the event base
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_EthNotify(data_eth_event_e event_id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(event_id: %d [%s])", __func__,
      event_id, GET_DATA_ETH_EVENT_NAME(event_id));

  switch (event_id) {
    case DATA_ETH_EVENT_CONNECTED: {
      ESP_LOGD(TAG, "Ethernet Link Up");
      sysctrl_InitSNTP();
      sysctrl_WaitTime();
      break;
    }
    case DATA_ETH_EVENT_DISCONNECTED: {
      ESP_LOGD(TAG, "Ethernet Link Down");
      break;
    }
    case DATA_ETH_EVENT_START: {
      ESP_LOGD(TAG, "Ethernet Started");
      break;
    }
    case DATA_ETH_EVENT_STOP: {
      ESP_LOGD(TAG, "Ethernet Stopped");
      break;
    }
    default: {
      break;
    }
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Parse incoming messages
 * 
 * @param msg Pointer to the message to be parsed
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_ParseMsg(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s], from: 0x%08lx, to: 0x%08lx)", __func__, 
      msg->type, GET_MSG_TYPE_NAME(msg->type),
      msg->from, msg->to);

  switch (msg->type) {
    case MSG_TYPE_INIT: {
      result = ESP_TASK_INIT;
      break;
    }
    case MSG_TYPE_DONE: {
      result = ESP_TASK_DONE;
      break;
    }
    case MSG_TYPE_RUN: {
      result = ESP_TASK_RUN;
      break;
    }

    case MSG_TYPE_ETH_EVENT: {
      sysctrl_EthNotify(msg->payload.eth.u.event_id);
      break;
    }

    default: {
      result = ESP_FAIL;
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief System task's function
 * 
 * @param param 
 */
static void sysctrl_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    sysctrl_GetTime();
    if(xQueueReceive(sys_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);
      
      result = sysctrl_ParseMsg(&msg);
      if (result == ESP_TASK_DONE) {
        loop = false;
        result = ESP_OK;
      }

      if (result != ESP_OK) {
        // TODO - Send Error to the Broker
        ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Message error.", __func__);
    }
  }
  if (sys_sem_id) {
    xSemaphoreGive(sys_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Send message to System controller
 * 
 * @param msg Pointer to the message to be sent
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(sys_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init System controller
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  sysctrl_GetTimeZone();

  /* Set timezone to Dallas Standard Time
    This is a TZ string in POSIX format that sets the time zone and DST rules.

    In short:
    - "CST6CDT" — the standard name CST (Central Standard Time) and the DST name CDT.
                  The number 6 means an offset of UTC−6 (local time is 6 hours behind UTC).
    - "M3.2.0/2" — start of daylight saving time: month 3 (March), second week, day 0 (Sunday), at 02:00.
    - "M11.1.0/2" — end of daylight saving time: month 11 (November), first week, day 0 (Sunday), at 02:00.

    So: we set the time zone to US Central (e.g. Dallas) with DST switching from the second Sunday in March at 02:00
        to the first Sunday in November at 02:00.

    The function sysctrl_setTimeZone applies this with setenv("TZ", ...) and tzset(), so local time functions
    (localtime, strftime) will use those rules.
  */
  sysctrl_setTimeZone("CST6CDT,M3.2.0/2,M11.1.0/2");

  /* Initialization message queue */
  sys_msg_queue = xQueueCreate(SYS_MSG_MAX, sizeof(msg_t));
  if (sys_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  sys_sem_id = xSemaphoreCreateCounting(1, 0);
  if (sys_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization thread */
  xTaskCreate(sysctrl_TaskFn, SYS_TASK_NAME, SYS_TASK_STACK_SIZE, NULL, SYS_TASK_PRIORITY, &sys_task_id);
  if (sys_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  /* Get ESP_MAC_BASE and keep it in sys_esp_mac */
  /* The default base MAC is pre-programmed by Espressif in eFuse BLK0. */
  if (sysctrl_GetMacAddress(sys_esp_mac, ESP_MAC_BASE) != ESP_OK) {
    ESP_LOGE(TAG, "[%s] sysctrl_GetMacAddress() failed.", __func__);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done System controller
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (sys_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_SYS_CTRL,
      .to = REG_SYS_CTRL,
    };
    result = sysctrl_Send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(sys_sem_id, portMAX_DELAY);

    vSemaphoreDelete(sys_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (sys_msg_queue) {
    vQueueDelete(sys_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run System controller
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
static esp_err_t sysctrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init System controller
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t SysCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_SYS_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = sysctrl_Init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done System controller
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t SysCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = sysctrl_Done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run System controller
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t SysCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = sysctrl_Run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Send message to the System controller thread
 * 
 * \return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t SysCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = sysctrl_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
