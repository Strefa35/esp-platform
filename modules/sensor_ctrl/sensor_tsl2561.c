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

#include "cJSON.h"

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

typedef enum {
  DATA_TYPE_UNKNOWN,
  DATA_TYPE_THRESHOLD,
  DATA_TYPE_LUX,
  DATA_TYPE_INFO,

  DATA_TYPE_MAX
} data_type_e;

typedef struct {
  uint16_t  lux;
  uint8_t   cnt;
  uint8_t   max;
  bool      on;
  bool      last_on;
} threshold_t;

static const char* TAG = "ESP::SENSORS::TSL2561";

static sensor_cb_f  tsl2561_cb = NULL;

static threshold_t tsl2561_threshold = {
  .lux = 1000,
  .cnt = 0,
  .max = 5,
  .on = false,
};

static uint32_t tsl2561_lux = 0;

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

  bool on = false;

  ESP_ERROR_CHECK(result = tsl2561_GetLux(handle, &tsl2561_lux));
  ESP_LOGD(TAG, "[%s] LUX: %ld", __func__, tsl2561_lux);

  /* Initial Threshold Setting */
  if (tsl2561_lux > tsl2561_threshold.lux) {
    on = true;
  }
  tsl2561_threshold.on = on;
  tsl2561_threshold.last_on = !on;

  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait... %d ms\n\n", __func__, POLLING_TIME_IN_MS);
    vTaskDelay(pdMS_TO_TICKS(POLLING_TIME_IN_MS));

    ESP_ERROR_CHECK(result = tsl2561_GetLux(handle, &tsl2561_lux));
    ESP_LOGD(TAG, "[%s] LUX: %ld", __func__, tsl2561_lux);
    if (tsl2561_lux > tsl2561_threshold.lux) {
      on = true;
    } else {
      on = false;
    }

    if (on == tsl2561_threshold.on) {
      ++tsl2561_threshold.cnt;
      ESP_LOGV(TAG, "[%s] KEEP -> cnt: %d, on: %d", __func__, tsl2561_threshold.cnt, on);
    } else {
      /* when level changed then reset tsl2561_threshold.cnt and wait to tsl2561_threshold.max */
      tsl2561_threshold.on = on;
      tsl2561_threshold.cnt = 0;
      ESP_LOGV(TAG, "[%s] RESET -> cnt: %d, on: %d", __func__, tsl2561_threshold.cnt, on);
    }

    /* when level stay during tsl2561_threshold.max then notify about change level */
    if (tsl2561_threshold.cnt >= tsl2561_threshold.max) {
      ESP_LOGV(TAG, "[%s] tsl2561_threshold ==> Lux: %ld, max: %d, on: %d", __func__, tsl2561_lux, tsl2561_threshold.lux, on);
      tsl2561_threshold.cnt = 0;
      /* if current level is different that last the notify */
      if (on != tsl2561_threshold.last_on) {
        ESP_LOGV(TAG, "[%s] LEVEL -> %d -> %d", __func__, tsl2561_threshold.last_on, on);
        tsl2561_threshold.last_on = on;
        if (tsl2561_cb) {

          data.dtype = SENSOR_DATA_LUX;
          data.u.uint32[0] = tsl2561_lux;

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

static esp_err_t sensorSetThreshold(const cJSON* data, cJSON* response) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(data: %p, response: %p)", __func__, data, response);
  if (data && cJSON_IsNumber(data)) {
    /* get threshold from JSON */
    int threshold = cJSON_GetNumberValue(data);
    ESP_LOGD(TAG, "[%s] threshold: %d", __func__, threshold);
    tsl2561_threshold.lux = threshold;
    tsl2561_threshold.cnt = 0;

    result = ESP_OK;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorGetThreshold(cJSON* response) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(response: %p)", __func__, response);
  if (response) {
    result = cJSON_AddNumberToObject(response, "threshold", tsl2561_threshold.lux) ? ESP_OK : ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorGetLux(cJSON* response) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(response: %p)", __func__, response);
  if (response) {
    result = cJSON_AddNumberToObject(response, "lux", tsl2561_lux) ? ESP_OK : ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorGetInfo(cJSON* response) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(response: %p)", __func__, response);
  if (response) {
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorSetDataType(const cJSON* item, const cJSON* type, cJSON* response) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(item: %p, type: %p, response: %p)", __func__, item, type, response);
  if (type) {
    char* type_str = cJSON_GetStringValue(type);

    if (type_str) {
      const cJSON* data = cJSON_GetObjectItem(item, type_str);

      ESP_LOGD(TAG, "[%s] type: '%s'", __func__, type_str);
      if (strcmp(type_str, "threshold") == 0) {
        result = sensorSetThreshold(data, response);
      }
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorGetDataType(const cJSON* type, cJSON* response) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(type: %p, response: %p)", __func__, type, response);
  if (type) {
    char* type_str = cJSON_GetStringValue(type);

    if (type_str) {
      ESP_LOGD(TAG, "[%s] type: '%s'", __func__, type_str);
      
      cJSON* item = cJSON_CreateObject();
      if (item) {
        cJSON_AddStringToObject(item, "type", type_str);
        if (strcmp(type_str, "threshold") == 0) {
          result = sensorGetThreshold(item);
        } else if (strcmp(type_str, "lux") == 0) {
          result = sensorGetLux(item);
        } else if (strcmp(type_str, "info") == 0) {
          result = sensorGetInfo(item);
        }
        if (result == ESP_OK) {
          cJSON_AddItemToArray(response, item);
        } else {
          cJSON_Delete(item);
        }
      }
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorSetItem(const cJSON* item, cJSON* response) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(item: %p, response: %p)", __func__, item, response);
  if (item) {
    result = sensorSetDataType(item, cJSON_GetObjectItem(item, "type"), response);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorSet(const cJSON* data, cJSON* response) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(data: %p, response: %p)", __func__, data, response);
  int array_cnt = cJSON_GetArraySize(data);
  if (array_cnt) {
    for (int idx = 0; idx < array_cnt; ++idx) {
      result = sensorSetItem(cJSON_GetArrayItem(data, idx), response);
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorGet(const cJSON* data, cJSON* response) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(data: %p, response: %p)", __func__, data, response);
  int array_cnt = cJSON_GetArraySize(data);
  if (array_cnt) {
    cJSON* resp_array = cJSON_AddArrayToObject(response, "data");
    for (int idx = 0; idx < array_cnt; ++idx) {
      result = sensorGetDataType(cJSON_GetArrayItem(data, idx), resp_array);
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t sensorInit(const sensor_cb_f cb) {
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

/*-----------------------------------------------------------------
  API functions
-----------------------------------------------------------------*/

esp_err_t sensor_InitTsl2561(const sensor_cb_f cb) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_SENSOR_TSL2561_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s(cb: %p)", __func__, cb);
  result = sensorInit(cb);
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

esp_err_t sensor_SetTsl2561(const cJSON* data, cJSON* response) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(data: %p, response: %p)", __func__, data, response);
  result = sensorSet(data, response);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t sensor_GetTsl2561(const cJSON* data, cJSON* response) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(data: %p, response: %p)", __func__, data, response);
  result = sensorGet(data, response);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
