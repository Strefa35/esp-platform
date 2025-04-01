/**
 * @file sensor_tsl2561.c
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor TSL2561
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "esp_log.h"

#include "sensor_ctrl.h"
#include "sensor_tsl2561.h"

#include "tsl2561.h"

#include "err.h"
#include "msg.h"


#define TASK_NAME               "tsl2561-task"
#define TASK_STACK_SIZE         4096
#define TASK_PRIORITY           20

#define POLLING_TIME_IN_MS      (1000)


typedef struct {
  uint16_t  lux;
  uint8_t   cnt;
  uint8_t   max;
  bool      on;
  bool      last_on;
} threshold_t;

static const char* TAG = "ESP::SENSORS::TSL2561";

static sensor_cb_f  tsl2561_cb = NULL;


/**
 * @brief TSL2561 task
 * 
 * @param param 
 */
static void taskFn(void* param) {
  tsl2561_t handle = NULL;
  bool loop = true;

  bool     power = false;
  uint8_t  id = 0;
  uint32_t lux = 0;

  sensor_data_t data = {
    .type = SENSOR_TYPE_TSL2561
  };

  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_ERROR_CHECK(tsl2561_Init(&handle));
  ESP_ERROR_CHECK(tsl2561_GetPower(handle, &power));
  ESP_ERROR_CHECK(tsl2561_GetId(handle, &id));
  ESP_LOGD(TAG, "[%s] Power: %d", __func__, power);
  ESP_LOGD(TAG, "[%s]    Id: 0x%02X", __func__, id);

  threshold_t threshold = {
    .lux = 1000,
    .cnt = 0,
    .max = 5,
    .on = false,
  };
  bool on = false;

  ESP_ERROR_CHECK(result = tsl2561_GetLux(handle, &lux));
  ESP_LOGD(TAG, "[%s] LUX: %ld", __func__, lux);

  /* Initial Threshold Setting */
  if (lux > threshold.lux) {
    on = true;
  }
  threshold.on = on;
  threshold.last_on = !on;

  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait... %d ms\n\n", __func__, POLLING_TIME_IN_MS);
    vTaskDelay(pdMS_TO_TICKS(POLLING_TIME_IN_MS));

    ESP_ERROR_CHECK(result = tsl2561_GetLux(handle, &lux));
    ESP_LOGD(TAG, "[%s] LUX: %ld", __func__, lux);
    if (lux > threshold.lux) {
      on = true;
    } else {
      on = false;
    }

    if (on == threshold.on) {
      ++threshold.cnt;
      ESP_LOGV(TAG, "[%s] KEEP -> cnt: %d, on: %d", __func__, threshold.cnt, on);
    } else {
      /* when level changed then reset threshold.cnt and wait to threshold.max */
      threshold.on = on;
      threshold.cnt = 0;
      ESP_LOGV(TAG, "[%s] RESET -> cnt: %d, on: %d", __func__, threshold.cnt, on);
    }

    /* when level stay during threshold.max then notify about change level */
    if (threshold.cnt >= threshold.max) {
      ESP_LOGV(TAG, "[%s] THRESHOLD ==> Lux: %ld, max: %d, on: %d", __func__, lux, threshold.lux, on);
      threshold.cnt = 0;
      /* if current level is different that last the notify */
      if (on != threshold.last_on) {
        ESP_LOGV(TAG, "[%s] LEVEL -> %d -> %d", __func__, threshold.last_on, on);
        threshold.last_on = on;
        if (tsl2561_cb) {

          data.dtype = SENSOR_DATA_LUX;
          data.u.uint32[0] = lux;

          result = tsl2561_cb(&data);
          if (result != ESP_OK) {
            ESP_LOGE(TAG, "[%s] tsl2561_cb(.dtype: %d) failed.", __func__, data.dtype);
          }
        }
      }
    }
  }
  ESP_ERROR_CHECK(tsl2561_SetPower(handle, false));
  ESP_ERROR_CHECK(tsl2561_Done(handle));

  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t init(const sensor_cb_f cb) {
  TaskHandle_t task_id = NULL;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(cb: %p)", __func__, cb);
  xTaskCreate(taskFn, TASK_NAME, TASK_STACK_SIZE, NULL, TASK_PRIORITY, &task_id);
  if (task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    result = ESP_FAIL;
  }
  tsl2561_cb = cb;
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t sensor_InitTsl2561(const sensor_cb_f cb) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_SENSOR_TSL2561_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s(cb: %p)", __func__, cb);
  result = init(cb);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t sensor_DoneTsl2561(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t sensor_RunTsl2561(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t sensor_SetTsl2561(const sensor_data_t* data) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(data: %p)", __func__, data);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t sensor_GetTsl2561(sensor_data_t* data) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(data: %p)", __func__, data);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}


