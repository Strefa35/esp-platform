/**
 * @file sensor_tsl2561.c
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor TSL2561
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "err.h"
#include "msg.h"
#include "sensors_ctrl.h"
#include "sensor_tsl2561.h"

#include "tsl2561.h"


static const char* TAG = "ESP::SENSORS::TSL2561";

esp_err_t tsl2561_InitSensor(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_SENSOR_TSL2561_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
