/**
 * @file ns2009.c
 * @author A.Czerwinski@pistacje.net
 * @brief TFT LCD Single Chip Driver
 * @version 0.1
 * @date 2025-02-22
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "sdkconfig.h"

#include "driver/i2c_master.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"

#include "ns2009.h"


#ifndef CONFIG_LCD_NS2009_TOUCH_THRESHOLD
#define CONFIG_LCD_NS2009_TOUCH_THRESHOLD 20
#endif
#ifndef CONFIG_LCD_NS2009_REQUIRE_PRESSURE
#define CONFIG_LCD_NS2009_REQUIRE_PRESSURE 0
#endif
#ifndef CONFIG_LCD_NS2009_RAW_MIN_X
#define CONFIG_LCD_NS2009_RAW_MIN_X 180
#endif
#ifndef CONFIG_LCD_NS2009_RAW_MAX_X
#define CONFIG_LCD_NS2009_RAW_MAX_X 3900
#endif
#ifndef CONFIG_LCD_NS2009_RAW_MIN_Y
#define CONFIG_LCD_NS2009_RAW_MIN_Y 220
#endif
#ifndef CONFIG_LCD_NS2009_RAW_MAX_Y
#define CONFIG_LCD_NS2009_RAW_MAX_Y 3900
#endif
#ifndef CONFIG_LCD_NS2009_SWAP_XY
#define CONFIG_LCD_NS2009_SWAP_XY 0
#endif
#ifndef CONFIG_LCD_NS2009_INVERT_X
#define CONFIG_LCD_NS2009_INVERT_X 0
#endif
#ifndef CONFIG_LCD_NS2009_INVERT_Y
#define CONFIG_LCD_NS2009_INVERT_Y 0
#endif
#ifndef CONFIG_LCD_NS2009_I2C_PORT
#define CONFIG_LCD_NS2009_I2C_PORT 0
#endif
#ifndef CONFIG_LCD_NS2009_SDA_GPIO
#define CONFIG_LCD_NS2009_SDA_GPIO 13
#endif
#ifndef CONFIG_LCD_NS2009_SCL_GPIO
#define CONFIG_LCD_NS2009_SCL_GPIO 16
#endif
#ifndef CONFIG_LCD_NS2009_I2C_FREQ_HZ
#define CONFIG_LCD_NS2009_I2C_FREQ_HZ 100000
#endif


#define NS2009_PORT_NUMBER             CONFIG_LCD_NS2009_I2C_PORT
#define NS2009_SLAVE_ADDR              0x48

#define NS2009_CMD_READ_X              0xC0
#define NS2009_CMD_READ_Y              0xD0
#define NS2009_CMD_READ_Z1             0xE0

#define NS2009_SDA_GPIO                CONFIG_LCD_NS2009_SDA_GPIO
#define NS2009_SCL_GPIO                CONFIG_LCD_NS2009_SCL_GPIO
#define NS2009_I2C_CLK_FREQUENCY       CONFIG_LCD_NS2009_I2C_FREQ_HZ

#define NS2009_TOUCH_THRESHOLD         CONFIG_LCD_NS2009_TOUCH_THRESHOLD

#define NS2009_RAW_MIN_X               CONFIG_LCD_NS2009_RAW_MIN_X
#define NS2009_RAW_MAX_X               CONFIG_LCD_NS2009_RAW_MAX_X
#define NS2009_RAW_MIN_Y               CONFIG_LCD_NS2009_RAW_MIN_Y
#define NS2009_RAW_MAX_Y               CONFIG_LCD_NS2009_RAW_MAX_Y


static const i2c_master_bus_config_t ns2009_bus_config = {
  .clk_source = I2C_CLK_SRC_DEFAULT,
  .i2c_port = NS2009_PORT_NUMBER,
  .scl_io_num = NS2009_SCL_GPIO,
  .sda_io_num = NS2009_SDA_GPIO,
  .glitch_ignore_cnt = 7,
  .flags.enable_internal_pullup = true,
};

static const i2c_device_config_t ns2009_dev_cfg = {
  .dev_addr_length = I2C_ADDR_BIT_LEN_7,
  .device_address = NS2009_SLAVE_ADDR,
  .scl_speed_hz = NS2009_I2C_CLK_FREQUENCY,
};


static i2c_master_bus_handle_t ns2009_i2c_bus_handle = NULL;
static i2c_master_dev_handle_t ns2009_i2c_dev_handle = NULL;
static bool ns2009_bus_owner = false;
static bool ns2009_dev_added = false;

static ns2009_res_t ns2009_res = {320, 240};

static const char* TAG = "ESP::LCD::NS2009";


/**
 * @brief Reads 12-bit value from NS2009 register.
 *
 * @param cmd Register read command.
 * @param value_out Output pointer for read value.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t ns2009_read_reg_u12(uint8_t cmd, uint16_t* value_out) {
  if (value_out == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  if (ns2009_i2c_dev_handle == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t rx[2] = {0};
  ESP_RETURN_ON_ERROR(
    i2c_master_transmit_receive(ns2009_i2c_dev_handle, &cmd, 1, rx, sizeof(rx), -1),
    TAG,
    "i2c tx/rx failed");

  *value_out = (uint16_t) ((rx[0] << 4) | (rx[1] >> 4));
  return ESP_OK;
}

/**
 * @brief Reads two samples from NS2009 register and returns average.
 *
 * @param cmd Register read command.
 * @param value_out Output pointer for averaged value.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t ns2009_read_reg_u12_avg(uint8_t cmd, uint16_t* value_out) {
  uint16_t v1 = 0;
  uint16_t v2 = 0;

  ESP_RETURN_ON_ERROR(ns2009_read_reg_u12(cmd, &v1), TAG, "first sample failed");
  ESP_RETURN_ON_ERROR(ns2009_read_reg_u12(cmd, &v2), TAG, "second sample failed");

  *value_out = (uint16_t) ((v1 + v2) / 2U);
  return ESP_OK;
}

/**
 * @brief Maps raw touch value from calibrated range to screen axis.
 *
 * @param raw Raw ADC value.
 * @param raw_min Minimum calibrated raw value.
 * @param raw_max Maximum calibrated raw value.
 * @param out_max Output axis size.
 * @return int32_t Mapped axis coordinate.
 */
static int32_t ns2009_map_to_axis(uint16_t raw, int32_t raw_min, int32_t raw_max, int32_t out_max) {
  if (raw <= raw_min) {
    return 0;
  }
  if (raw >= raw_max) {
    return out_max - 1;
  }

  int32_t num = ((int32_t) raw - raw_min) * (out_max - 1);
  int32_t den = (raw_max - raw_min);
  if (den <= 0) {
    return 0;
  }
  return num / den;
}

/**
 * @brief Initializes NS2009 I2C bus and touch device handle.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t ns2009_init(void) {
  ESP_LOGI(TAG, "++%s()", __func__);

  esp_err_t result = i2c_new_master_bus(&ns2009_bus_config, &ns2009_i2c_bus_handle);
  if (result == ESP_OK) {
    ns2009_bus_owner = true;
  } else if (result == ESP_ERR_INVALID_STATE) {
    result = i2c_master_get_bus_handle(NS2009_PORT_NUMBER, &ns2009_i2c_bus_handle);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] i2c_master_get_bus_handle() failed: %d", __func__, result);
      return result;
    }
    ns2009_bus_owner = false;
  } else {
    ESP_LOGE(TAG, "[%s] i2c_new_master_bus() failed: %d", __func__, result);
    return result;
  }

  result = i2c_master_bus_add_device(ns2009_i2c_bus_handle, &ns2009_dev_cfg, &ns2009_i2c_dev_handle);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_master_bus_add_device() failed: %d", __func__, result);
    return result;
  }
  ns2009_dev_added = true;

  result = i2c_master_probe(ns2009_i2c_bus_handle, NS2009_SLAVE_ADDR, 100 / portTICK_PERIOD_MS);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_master_probe() failed: %d", __func__, result);
    return result;
  }

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}

/**
 * @brief Deinitializes NS2009 I2C device and bus handles.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t ns2009_done(void) {
  ESP_LOGI(TAG, "++%s()", __func__);

  if (ns2009_i2c_dev_handle != NULL && ns2009_dev_added) {
    i2c_master_bus_rm_device(ns2009_i2c_dev_handle);
    ns2009_i2c_dev_handle = NULL;
    ns2009_dev_added = false;
  }

  if (ns2009_i2c_bus_handle != NULL && ns2009_bus_owner) {
    i2c_del_master_bus(ns2009_i2c_bus_handle);
    ns2009_bus_owner = false;
  }
  ns2009_i2c_bus_handle = NULL;

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}

/**
 * @brief Initializes NS2009 driver with display resolution.
 *
 * @param res Pointer to display resolution.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t ns2009_Init(const ns2009_res_t* res) {
  esp_log_level_set(TAG, CONFIG_LCD_NS2009_LOG_LEVEL);

  if (res == NULL || res->h == 0 || res->v == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "++%s(h: %ld, v: %ld)", __func__, res->h, res->v);
  ns2009_res = *res;

  esp_err_t result = ns2009_init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Deinitializes NS2009 driver resources.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t ns2009_Done(void) {
  ESP_LOGI(TAG, "++%s()", __func__);
  esp_err_t result = ns2009_done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Reads and converts current touch sample to screen coordinates.
 *
 * @param touch Output structure with mapped touch coordinates.
 * @return esp_err_t ESP_OK when touch is valid, error code otherwise.
 */
esp_err_t ns2009_GetTouch(ns2009_touch_t* touch) {
  if (touch == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  if (ns2009_i2c_dev_handle == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  uint16_t raw_z1 = 0;
  uint16_t raw_x = 0;
  uint16_t raw_y = 0;

  ESP_RETURN_ON_ERROR(ns2009_read_reg_u12_avg(NS2009_CMD_READ_X, &raw_x), TAG, "read x failed");
  ESP_RETURN_ON_ERROR(ns2009_read_reg_u12_avg(NS2009_CMD_READ_Y, &raw_y), TAG, "read y failed");

  bool in_raw_range =
    (raw_x >= (uint16_t) NS2009_RAW_MIN_X) &&
    (raw_x <= (uint16_t) NS2009_RAW_MAX_X) &&
    (raw_y >= (uint16_t) NS2009_RAW_MIN_Y) &&
    (raw_y <= (uint16_t) NS2009_RAW_MAX_Y);

  if (!in_raw_range) {
    return ESP_ERR_NOT_FOUND;
  }

  if (ns2009_read_reg_u12_avg(NS2009_CMD_READ_Z1, &raw_z1) == ESP_OK) {
#if CONFIG_LCD_NS2009_REQUIRE_PRESSURE
    if (raw_z1 < NS2009_TOUCH_THRESHOLD) {
      return ESP_ERR_NOT_FOUND;
    }
#endif
  }

  uint16_t raw_x_cal = raw_x;
  uint16_t raw_y_cal = raw_y;

#if CONFIG_LCD_NS2009_SWAP_XY
  uint16_t tmp = raw_x_cal;
  raw_x_cal = raw_y_cal;
  raw_y_cal = tmp;
#endif

  int32_t x = ns2009_map_to_axis(raw_x_cal, NS2009_RAW_MIN_X, NS2009_RAW_MAX_X, (int32_t) ns2009_res.h);
  int32_t y = ns2009_map_to_axis(raw_y_cal, NS2009_RAW_MIN_Y, NS2009_RAW_MAX_Y, (int32_t) ns2009_res.v);

#if CONFIG_LCD_NS2009_INVERT_X
  x = ((int32_t) ns2009_res.h - 1) - x;
#endif
#if CONFIG_LCD_NS2009_INVERT_Y
  y = ((int32_t) ns2009_res.v - 1) - y;
#endif

  touch->x = x;
  touch->y = y;
  touch->z = raw_z1;

  return ESP_OK;
}