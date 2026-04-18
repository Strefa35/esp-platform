/**
 * @file cli_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief CLI Controller (ESP-IDF console REPL and module command registration hooks)
 * @version 0.1
 * @date 2025-02-12
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 *
 */
#include <stdio.h>
#include <string.h>

#include "esp_console.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "err.h"
#include "lut.h"
#include "msg.h"
#include "cli_ctrl.h"

#if CONFIG_WIFI_CTRL_ENABLE
#include "cli_wifi.h"
#endif
#if CONFIG_CLI_CTRL_ENABLE && CONFIG_LCD_CTRL_ENABLE
#include "cli_lcd.h"
#endif

#define CLI_TASK_NAME           "cli-task"
#define CLI_TASK_STACK_SIZE     4096
#define CLI_TASK_PRIORITY       12

#define CLI_MSG_MAX             8

#if CONFIG_CLI_CTRL_ENABLE
#if !CONFIG_CLI_CTRL_REPL_STACK_SIZE
#define CLI_CTRL_REPL_STACK_SIZE 8192
#else
#define CLI_CTRL_REPL_STACK_SIZE CONFIG_CLI_CTRL_REPL_STACK_SIZE
#endif
#if !CONFIG_CLI_CTRL_REPL_TASK_PRIORITY
#define CLI_CTRL_REPL_TASK_PRIORITY 2
#else
#define CLI_CTRL_REPL_TASK_PRIORITY CONFIG_CLI_CTRL_REPL_TASK_PRIORITY
#endif
#endif

static const char *TAG = "ESP::CLI";

static QueueHandle_t      cli_msg_queue = NULL;
static TaskHandle_t       cli_task_id = NULL;
static SemaphoreHandle_t  cli_sem_id = NULL;

#if CONFIG_CLI_CTRL_ENABLE
static esp_console_repl_t *s_repl = NULL;
static bool                s_console_inited = false;
#endif

/**
 * @brief Handle one control message dequeued by the CLI module task.
 *
 * Maps lifecycle types to `ESP_TASK_*` codes used by the manager pattern.
 *
 * @param msg Incoming message from `cli_msg_queue`.
 *
 * @return `ESP_TASK_INIT`, `ESP_TASK_RUN`, `ESP_TASK_DONE`, or `ESP_FAIL` for unsupported types.
 */
static esp_err_t clictrl_ParseMsg(const msg_t *msg)
{
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s], from: 0x%08lx, to: 0x%08lx)", __func__,
           msg->type, GET_MSG_TYPE_NAME(msg->type), (unsigned long)msg->from, (unsigned long)msg->to);

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
    default: {
      result = ESP_FAIL;
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, (int)result);
  return result;
}

/**
 * @brief FreeRTOS task body: block on `cli_msg_queue` and run `clictrl_ParseMsg()`.
 *
 * Signals `cli_sem_id` when exiting after `MSG_TYPE_DONE`.
 *
 * @param param Unused (NULL).
 */
static void clictrl_TaskFn(void *param)
{
  (void)param;
  msg_t     msg;
  bool      loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if (xQueueReceive(cli_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__,
               msg.type, GET_MSG_TYPE_NAME(msg.type), (unsigned long)msg.from, (unsigned long)msg.to);

      result = clictrl_ParseMsg(&msg);
      if (result == ESP_TASK_DONE) {
        loop   = false;
        result = ESP_OK;
      }

      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] Error: %d", __func__, (int)result);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Message error.", __func__);
    }
  }
  if (cli_sem_id) {
    xSemaphoreGive(cli_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Non-blocking post of a message to `cli_msg_queue` (zero tick timeout).
 *
 * @param msg Message to copy into the queue; must not be NULL.
 *
 * @return ESP_OK if queued, ESP_FAIL if the queue is full or unavailable.
 */
static esp_err_t clictrl_Send(const msg_t *msg)
{
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(cli_msg_queue, msg, (TickType_t)0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type,
             (unsigned long)msg->from, (unsigned long)msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, (int)result);
  return result;
}

/**
 * @brief Create the CLI message queue, shutdown semaphore, and worker task.
 *
 * @return ESP_OK on success, ESP_FAIL if any RTOS object or task creation fails.
 */
static esp_err_t clictrl_Init(void)
{
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  cli_msg_queue = xQueueCreate(CLI_MSG_MAX, sizeof(msg_t));
  if (cli_msg_queue == NULL) {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  cli_sem_id = xSemaphoreCreateCounting(1, 0);
  if (cli_sem_id == NULL) {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  if (xTaskCreate(clictrl_TaskFn, CLI_TASK_NAME, CLI_TASK_STACK_SIZE, NULL, CLI_TASK_PRIORITY, &cli_task_id) != pdPASS) {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, (int)result);
  return result;
}

#if CONFIG_CLI_CTRL_ENABLE
/**
 * @brief Tear down the ESP console REPL and console module state.
 *
 * Stops the REPL task via `esp_console_stop_repl()`, then calls `esp_console_deinit()`.
 *
 * @return ESP_OK (warnings are logged on non-fatal `esp_err_t` from IDF APIs).
 */
static esp_err_t clictrl_StopConsole(void)
{
  if (s_repl != NULL) {
    esp_err_t err = esp_console_stop_repl(s_repl);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "esp_console_stop_repl: %s", esp_err_to_name(err));
    }
    s_repl = NULL;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  if (s_console_inited) {
    esp_err_t err = esp_console_deinit();
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "esp_console_deinit: %s", esp_err_to_name(err));
    }
    s_console_inited = false;
  }
  return ESP_OK;
}
#endif

/**
 * @brief Shut down console (if enabled), stop the CLI worker task, and delete RTOS objects.
 *
 * Posts `MSG_TYPE_DONE` to the worker queue and waits on `cli_sem_id` for task exit.
 *
 * @return Result of the final `clictrl_Send()` for the DONE message, or ESP_OK if semaphore was missing.
 */
static esp_err_t clictrl_Done(void)
{
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

#if CONFIG_CLI_CTRL_ENABLE
  (void)clictrl_StopConsole();
#endif

  if (cli_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_CLI_CTRL,
      .to   = REG_CLI_CTRL,
    };
    result = clictrl_Send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(cli_sem_id, portMAX_DELAY);

    vSemaphoreDelete(cli_sem_id);
    cli_sem_id = NULL;
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);
  }
  if (cli_msg_queue) {
    vQueueDelete(cli_msg_queue);
    cli_msg_queue = NULL;
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  cli_task_id = NULL;

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, (int)result);
  return result;
}

/**
 * @brief Start the interactive console REPL when `CONFIG_CLI_CTRL_ENABLE` is set.
 *
 * Initializes the console REPL via `esp_console_new_repl_*` (which calls `esp_console_init` and
 * registers `help` internally). Module-specific commands (e.g. Wi-Fi) register via small hooks after the REPL exists.
 * Do not call `esp_console_init()` before `esp_console_new_repl_*` — a second init returns
 * `ESP_ERR_INVALID_STATE` and the error path tears down the UART, breaking `ESP_LOG` / stdout.
 *
 * @return ESP_OK on success or when CLI is disabled; an `esp_err_t` from IDF on failure; ESP_ERR_NOT_SUPPORTED
 *         if no console backend is selected.
 */
static esp_err_t clictrl_Run(void)
{
  ESP_LOGI(TAG, "++%s()", __func__);

#if !CONFIG_CLI_CTRL_ENABLE
  ESP_LOGI(TAG, "--%s() - CLI disabled in Kconfig", __func__);
  return ESP_OK;
#else

  if (s_repl != NULL) {
    ESP_LOGI(TAG, "--%s() - REPL already running", __func__);
    return ESP_OK;
  }

  esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_cfg.prompt             = CONFIG_CLI_CTRL_PROMPT_STRING;
  repl_cfg.task_stack_size    = CLI_CTRL_REPL_STACK_SIZE;
  repl_cfg.task_priority      = CLI_CTRL_REPL_TASK_PRIORITY;
  repl_cfg.max_cmdline_length = 0;

  esp_err_t err;

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
  esp_console_dev_uart_config_t uart_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  err                                    = esp_console_new_repl_uart(&uart_cfg, &repl_cfg, &s_repl);
#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  esp_console_dev_usb_serial_jtag_config_t usb_cfg = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
  err = esp_console_new_repl_usb_serial_jtag(&usb_cfg, &repl_cfg, &s_repl);
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
  esp_console_dev_usb_cdc_config_t cdc_cfg = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
  err                                      = esp_console_new_repl_usb_cdc(&cdc_cfg, &repl_cfg, &s_repl);
#else
  ESP_LOGE(TAG, "No supported CONFIG_ESP_CONSOLE_* backend for REPL");
  (void)clictrl_StopConsole();
  return ESP_ERR_NOT_SUPPORTED;
#endif

  if (err != ESP_OK || s_repl == NULL) {
    ESP_LOGE(TAG, "esp_console_new_repl_*: %s", esp_err_to_name(err));
    (void)clictrl_StopConsole();
    return (err != ESP_OK) ? err : ESP_FAIL;
  }

  s_console_inited = true;

#if CONFIG_WIFI_CTRL_ENABLE
  CliWifi_RegisterConsoleCmd();
#endif
#if CONFIG_CLI_CTRL_ENABLE && CONFIG_LCD_CTRL_ENABLE
  CliLcd_RegisterConsoleCmd();
#endif

  err = esp_console_start_repl(s_repl);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_console_start_repl: %s", esp_err_to_name(err));
    (void)clictrl_StopConsole();
    return err;
  }

  ESP_LOGI(TAG, "Console REPL started (type 'help')");
  ESP_LOGI(TAG, "--%s() - result: ESP_OK", __func__);
  return ESP_OK;
#endif /* CONFIG_CLI_CTRL_ENABLE */
}

/**
 * @brief Initialize the CLI controller (log level, queue, worker task).
 *
 * @return ESP_OK on success, ESP_FAIL if initialization fails.
 */
esp_err_t CliCtrl_Init(void)
{
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_CLI_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = clictrl_Init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, (int)result);
  return result;
}

/**
 * @brief Shut down the CLI controller and release all module resources.
 *
 * @return Value propagated from internal teardown (`clictrl_Done()`).
 */
esp_err_t CliCtrl_Done(void)
{
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = clictrl_Done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, (int)result);
  return result;
}

/**
 * @brief Run-phase entry: start the UART/USB REPL when enabled.
 *
 * @return Value propagated from `clictrl_Run()`.
 */
esp_err_t CliCtrl_Run(void)
{
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = clictrl_Run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, (int)result);
  return result;
}

/**
 * @brief Enqueue a control message for the CLI worker task.
 *
 * @param msg Message to send; must not be NULL.
 *
 * @return ESP_OK if posted, ESP_FAIL if the queue is full.
 */
esp_err_t CliCtrl_Send(const msg_t *msg)
{
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = clictrl_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, (int)result);
  return result;
}
