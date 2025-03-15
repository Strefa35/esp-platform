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
#include <stdio.h>
#include <stdbool.h>

#include "sdkconfig.h"

#include "driver/gpio.h"

#include "driver/i2c_master.h"

#include "esp_log.h"
#include "esp_err.h"

#include "ns2009.h"

//NS2009
#define NS2009_PORT_NUMBER          0
#define NS2009_SLAVE_ADDR           0x48 //10010000

#define NS2009_WRITE_ADDR           0x90
#define NS2009_READ_ADDR            0x91

#define NS2009_READ_X               0xc0
#define NS2009_READ_Y               0xd0
#define NS2009_READ_Z1              0xe0

#define NS2009_SDA_GPIO             13
#define NS2009_SCL_GPIO             16
//#define NS2009_I2C_CLK_FREQUENCY    400000
#define NS2009_I2C_CLK_FREQUENCY    100000

#define I2C_MASTER_TX_BUF_DISABLE       0
#define I2C_MASTER_RX_BUF_DISABLE       0

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

static ns2009_res_t ns2009_res = {320, 240};

static const char* TAG = "ESP::LCD::NS2009";

/**
 * @brief Initialize I2C bus and add device to the bus
 * 
 * @return esp_err_t 
 */
static esp_err_t ns2009_init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "Initialize I2C bus");
  result = i2c_new_master_bus(&ns2009_bus_config, &ns2009_i2c_bus_handle);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_new_master_bus() failed: %d.", __func__, result);
    return result;
  }

  ESP_ERROR_CHECK(i2c_master_probe(ns2009_i2c_bus_handle, NS2009_SLAVE_ADDR, -1));

  ESP_LOGI(TAG, "Add device to the bus");
  result = i2c_master_bus_add_device(ns2009_i2c_bus_handle, &ns2009_dev_cfg, &ns2009_i2c_dev_handle);

  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_master_bus_add_device() failed: %d.", __func__, result);
    return result;
  }

 
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Remove device from the bus and delete bus
 * 
 * @return esp_err_t 
 */
static esp_err_t ns2009_done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  if (ns2009_i2c_dev_handle) {
    result = i2c_master_bus_rm_device(ns2009_i2c_dev_handle);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] i2c_master_bus_rm_device() failed: %d.", __func__, result);
    }
    ns2009_i2c_dev_handle = NULL;
  }

  if (ns2009_i2c_bus_handle != NULL) {
    result = i2c_del_master_bus(ns2009_i2c_bus_handle);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] i2c_del_master_bus() failed: %d.", __func__, result);
    }
    ns2009_i2c_bus_handle = NULL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Write command to the I2C device
 * 
 * @param data_ptr - buffer with data to send
 * @param data_size - size of buffer / data to send
 * @return esp_err_t 
 */
static esp_err_t ns2009_write(const uint8_t* data_ptr, const size_t data_size) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(data_ptr: %p, data_size: %d)", __func__, data_ptr, data_size);

  if (ns2009_i2c_dev_handle == NULL) {

    ESP_LOGE(TAG, "[%s] i2c_master_bus_rm_device() failed: %d.", __func__, result);
    return ESP_FAIL;
  }

  result = i2c_master_transmit(ns2009_i2c_dev_handle, data_ptr, data_size, -1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_master_transmit() failed: %d.", __func__, result);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Read data from the I2C device
 * 
 * @param data_ptr - buffer to fill a data
 * @param data_size - size of buffer / data to read
 * @return esp_err_t 
 */
static esp_err_t ns2009_read(uint8_t* data_ptr, size_t data_size) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(data_ptr: %p, data_size: %d)", __func__, data_ptr, data_size);
  if (ns2009_i2c_dev_handle == NULL) {

    ESP_LOGE(TAG, "[%s] i2c_master_bus_rm_device() failed: %d.", __func__, result);
    return ESP_FAIL;
  }

  result = i2c_master_receive(ns2009_i2c_dev_handle, data_ptr, data_size, -1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_master_transmit() failed: %d.", __func__, result);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initialize NS2009 driver
 * 
 * @param res - Display size (touchpad size)
 * @return esp_err_t 
 */
esp_err_t ns2009_Init(const ns2009_res_t* res) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_LCD_NS2009_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s(h: %ld, v: %ld)", __func__, res->h, res->v);
  ns2009_res.h = res->h;
  ns2009_res.v = res->v;
  result = ns2009_init();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] ns2009_init() - result: %d.", __func__, result);
    return result;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t ns2009_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = ns2009_done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t ns2009_GetTouch(ns2009_touch_t* touch) {
  uint8_t   data[2];
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Check touch */

  /* ====================== */
  /*     Write command      */
  /* ====================== */
  /* Addr = 0x90 + Cmd = 0xC0 = 1100|0000 */
  data[0] = NS2009_WRITE_ADDR;
  data[1] = NS2009_READ_Z1;
  result = ns2009_write(data, 2);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] ns2009_write() - result: %d", __func__, result);
    return result;
  }

  /* ====================== */
  /*     Read command       */
  /* ====================== */
  /* Addr = 0x91 */
  data[0] = NS2009_READ_ADDR;
  result = ns2009_write(data, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] ns2009_write() - result: %d", __func__, result);
    return result;
  }
  /* Read data */
  result = ns2009_read(data, 2);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] ns2009_read() - result: %d", __func__, result);
    return result;
  }

  uint32_t  _touch = (data[1] << 8 | (data[0] & 0xF0) >> 4);

  ESP_LOGD(TAG, "[%s] -> TOUCH: %ld", __func__, _touch);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}