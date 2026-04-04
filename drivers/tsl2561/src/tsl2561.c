/**
 * @file tsl2561.c
 * @author A.Czerwinski@pistacje.net
 * @brief Light to digital converter for TSL2560 & TSL2561
 * @version 0.1
 * @date 2025-03-16
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"

#include "driver/i2c_master.h"

#include "esp_log.h"
#include "esp_err.h"

#include "tsl2561.h"
#include "tsl2561_reg.h"
#include "tsl2561_type.h"
#include "tsl2561_lux.h"


// TSL2561
#define TSL2561_PORT_NUMBER           (CONFIG_TSL2561_PORT_NUMBER)
#define TSL2561_SLAVE_ADDR            (CONFIG_TSL2561_SLAVE_ADDR)

#define TSL2561_I2C_CLK_FREQUENCY     (CONFIG_TSL2561_I2C_CLK_FREQUENCY * 1000) // in kHz

/* GPIO Configuration */
#define TSL2561_SDA_GPIO              (CONFIG_TSL2561_SDA_GPIO)
#define TSL2561_SCL_GPIO              (CONFIG_TSL2561_SCL_GPIO)
#define TSL2561_INT_GPIO              (CONFIG_TSL2561_INT_GPIO)

#define TSL2561_INTR_FLAG_DEFAULT     (0)


static const i2c_master_bus_config_t tsl2561_bus_config = {
  .clk_source = I2C_CLK_SRC_DEFAULT,
  .i2c_port = TSL2561_PORT_NUMBER,
  .scl_io_num = TSL2561_SCL_GPIO,
  .sda_io_num = TSL2561_SDA_GPIO,
  .glitch_ignore_cnt = 0,
  .flags.enable_internal_pullup = true,
};

static const i2c_device_config_t tsl2561_dev_cfg = {
  .dev_addr_length = I2C_ADDR_BIT_LEN_7,
  .device_address = TSL2561_SLAVE_ADDR,
  .scl_speed_hz = TSL2561_I2C_CLK_FREQUENCY,
};


static const char* TAG = "ESP::DRV::TLS256X";

static void tsl2561_print(const tsl2561_t handle) {
  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  ESP_LOGI(TAG, "       bus: %p", handle->bus);
  ESP_LOGI(TAG, "    device: %p", handle->device);
  ESP_LOGI(TAG, "    partno: %d", handle->partno);
  ESP_LOGI(TAG, "     ratio: %f", handle->ratio);
  ESP_LOGI(TAG, "     revno: %d", handle->revno);
  ESP_LOGI(TAG, "      gain: %d", handle->gain);
  ESP_LOGI(TAG, "     integ: %d", handle->integ);
  ESP_LOGI(TAG, "        ms: %d", handle->ms);
  ESP_LOGI(TAG, "isr_notify: %p", handle->isr_notify);
  ESP_LOGI(TAG, " threshold: [%d - %d]", handle->threshold.min, handle->threshold.min);
  ESP_LOGI(TAG, "--%s()", __func__);
}

static void tsl2561_isrHandler(void* arg) {
  tsl2561_t handle = (tsl2561_t) arg;

  if (handle && handle->isr_notify) {
    handle->isr_notify(handle);
  }
}

static esp_err_t tsl2561_setIsrNotify(tsl2561_t handle, const tsl2561_isr_f fn) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, fn: %p)", __func__, handle, fn);
  handle->isr_notify = fn;
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t tsl2561_setIsrConfig(tsl2561_t handle, const bool enable) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, enable: %d)", __func__, handle, enable);
  if (enable) { // enable ISR
    
    gpio_config_t int_conf = {
      .intr_type = GPIO_INTR_ANYEDGE,
      .mode = GPIO_MODE_INPUT,
      .pin_bit_mask = (1ULL << TSL2561_INT_GPIO),
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .pull_up_en = GPIO_PULLUP_ENABLE
    };

    result = gpio_config(&int_conf);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] gpio_config() - failed: %d.", __func__, result);
      return result;
    }
    result = gpio_install_isr_service(TSL2561_INTR_FLAG_DEFAULT);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] gpio_install_isr_service() - failed: %d.", __func__, result);
      return result;
    }
    result = gpio_isr_handler_add(TSL2561_INT_GPIO, tsl2561_isrHandler, (void*) handle);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] gpio_isr_handler_add() - failed: %d.", __func__, result);
      return result;
    }
    result = gpio_intr_enable(TSL2561_INT_GPIO);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] gpio_intr_enable() - failed: %d.", __func__, result);
      return result;
    }

  } else { // disable ISR
    result = gpio_isr_handler_remove(TSL2561_INT_GPIO);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] gpio_isr_handler_remove() - failed: %d.", __func__, result);
      return result;
    }
    result = gpio_intr_disable(TSL2561_INT_GPIO);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] gpio_intr_disable() - failed: %d.", __func__, result);
      return result;
    }
    //gpio_uninstall_isr_service();
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
static esp_err_t tsl2561_read(const tsl2561_t handle, uint8_t* data_ptr, size_t data_size) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, data_ptr: %p, data_size: %d)", __func__, handle, data_ptr, data_size);
  result = i2c_master_receive(handle->device, data_ptr, data_size, -1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_master_transmit() failed: %d.", __func__, result);
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
static esp_err_t tsl2561_write(const tsl2561_t handle, const uint8_t* data_ptr, const size_t data_size) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, data_ptr: %p, data_size: %d)", __func__, handle, data_ptr, data_size);
  result = i2c_master_transmit(handle->device, data_ptr, data_size, -1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_master_transmit() failed: %d.", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Set Power On/Off
 * 
 * @param handle 
 * @param on 
 * @return esp_err_t 
 */
static esp_err_t tsl2561_setPower(const tsl2561_t handle, const bool on) {
  uint8_t cmd = TSL2561_REG_COMMAND | TSL2561_REG_CONTROL;
  uint8_t data = (on == true ? TSL2561_CTRL_POWER_ON : TSL2561_CTRL_POWER_OFF);
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, on: %d)", __func__, handle, on);
  ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd);
  result = tsl2561_write(handle, &cmd, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(data: 0x%02X) - failed: %d.", __func__, data, result);
    return result;
  }
  ESP_LOGD(TAG, "[%s] -> data: 0x%02X", __func__, data);
  result = tsl2561_write(handle, &data, 1);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get Power state (On/Off)
 * 
 * @param handle 
 * @return esp_err_t 
 */
static esp_err_t tsl2561_getPower(const tsl2561_t handle, bool* on) {
  uint8_t cmd = TSL2561_REG_COMMAND | TSL2561_REG_CONTROL;
  uint8_t data;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd);
  result = tsl2561_write(handle, &cmd, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(cmd: 0x%02X) - failed: %d.", __func__, cmd, result);
    return result;
  }
  result = tsl2561_read(handle, &data, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_read() - failed: %d.", __func__, result);
    return result;
  }
  ESP_LOGD(TAG, "[%s] <- data: 0x%02X", __func__, data);
  *on = data & TSL2561_CTRL_POWER_ON ? true : false;
  ESP_LOGD(TAG, "--> POWER: 0x%02X", data);
  ESP_LOGI(TAG, "--%s(on: %d) - result: %d", __func__, *on, result);
  return result;
}

/**
 * @brief Read ID Register (PARTNO & REVNO)
 *
 * @param handle
 * @return esp_err_t
 */
static esp_err_t tsl2561_readId(const tsl2561_t handle) {
  uint8_t cmd = TSL2561_REG_COMMAND | TSL2561_REG_ID;
  uint8_t data;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd);
  result = tsl2561_write(handle, &cmd, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(cmd: 0x%02X) - failed: %d.", __func__, cmd, result);
    return result;
  }
  result = tsl2561_read(handle, &data, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_read() - failed: %d.", __func__, result);
    return result;
  }
  ESP_LOGD(TAG, "[%s] <- data: 0x%02X", __func__, data);
  handle->partno  = data & 0xF0;
  handle->revno   = data & 0x0F;
  ESP_LOGD(TAG, "--> REG: 0x%02X", data);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Read Timing Register (GAIN & INTEG)
 *
 * @param handle
 * @return esp_err_t
 */
static esp_err_t tsl2561_readTiming(const tsl2561_t handle) {
  uint8_t cmd = TSL2561_REG_COMMAND | TSL2561_REG_TIMING;
  uint8_t data;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd);
  result = tsl2561_write(handle, &cmd, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(cmd: 0x%02X) - failed: %d.", __func__, cmd, result);
    return result;
  }
  result = tsl2561_read(handle, &data, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_read() - failed: %d.", __func__, result);
    return result;
  }
  ESP_LOGD(TAG, "[%s] <- data: 0x%02X", __func__, data);
  handle->gain  = data & TSL2561_TIMING_GAIN;
  handle->integ = data & TSL2561_TIMING_INTEGRATE;
  switch (handle->integ) {
    case INTEG_13MS: {
      handle->ms = 20; // 13.7ms
      break;
    }
    case INTEG_101MS: {
      handle->ms = 150; // 101ms
      break;
    }
    case INTEG_402MS: {
      handle->ms = 450; // 402ms
      break;
    }
    default: {
        handle->ms = 450;
    }
  }
  ESP_LOGD(TAG, "--> REG: 0x%02X", data);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Write Gain
 *
 * @param handle
 * @param gain - tsl2561_gain_e
 * @return esp_err_t
 */
static esp_err_t tsl2561_writeGain(const tsl2561_t handle, const tsl2561_gain_e gain) {
  uint8_t cmd = TSL2561_REG_COMMAND | TSL2561_REG_TIMING;
  uint8_t data = handle->integ | gain;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, gain: 0x%02X)", __func__, handle, gain);
  ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd);
  result = tsl2561_write(handle, &cmd, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(cmd: 0x%02X) - failed: %d.", __func__, cmd, result);
    return result;
  }
  ESP_LOGD(TAG, "[%s] -> data: 0x%02X", __func__, data);
  result = tsl2561_write(handle, &data, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(data: 0x%02X) - failed: %d.", __func__, data, result);
    return result;
  }
  handle->gain  = gain;
  ESP_LOGD(TAG, "--> REG: 0x%02X", data);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Write Integration Time
 *
 * @param handle
 * @param time - tsl2561_integ_e
 * @return esp_err_t
 */
static esp_err_t tsl2561_writeIntegrationTime(const tsl2561_t handle, const tsl2561_integ_e time) {
  uint8_t cmd = TSL2561_REG_COMMAND | TSL2561_REG_TIMING;
  uint8_t data = handle->gain | time;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, time: 0x%02X)", __func__, handle, time);
  ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd);
  result = tsl2561_write(handle, &cmd, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(cmd: 0x%02X) - failed: %d.", __func__, cmd, result);
    return result;
  }
  ESP_LOGD(TAG, "[%s] -> data: 0x%02X", __func__, data);
  result = tsl2561_write(handle, &data, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(data: 0x%02X) - failed: %d.", __func__, data, result);
    return result;
  }
  handle->integ  = time;
  ESP_LOGD(TAG, "--> REG: 0x%02X", data);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Read Data from Channels
 *
 * @param handle
 * @return esp_err_t
 */
static esp_err_t tsl2561_readChannels(const tsl2561_t handle) {
  uint8_t cmd_ch0 = TSL2561_REG_COMMAND | TSL2561_CMD_WORD_PROTOCOL | TSL2561_REG_DATA_0;
  uint8_t cmd_ch1 = TSL2561_REG_COMMAND | TSL2561_CMD_WORD_PROTOCOL | TSL2561_REG_DATA_1;
  uint8_t data_ch0[2];
  uint8_t data_ch1[2];
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);

  /* Clear previous values */
  handle->broadband = 0;
  handle->infrared = 0;

  ESP_LOGD(TAG, "[%s] Wait %dms", __func__, handle->ms);
  vTaskDelay(pdMS_TO_TICKS(handle->ms));

  /* Read data from channel 0 */
  ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd_ch0);
  result = tsl2561_write(handle, &cmd_ch0, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(cmd_ch0: 0x%02X) - failed: %d.", __func__, cmd_ch0, result);
    return result;
  }
  result = tsl2561_read(handle, data_ch0, 2);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_read() - failed: %d.", __func__, result);
    return result;
  }
  ESP_LOGD(TAG, "[%s] <- data: 0x%02X%02X", __func__, data_ch1[1], data_ch1[0]);
  handle->broadband = (data_ch0[1] << 8) | data_ch0[0];
  ESP_LOGD(TAG, "--> CH0: 0x%04X [%d]", handle->broadband, handle->broadband);

  /* Read data from channel 1 */
  ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd_ch1);
  result = tsl2561_write(handle, &cmd_ch1, 1);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_write(cmd_ch1: 0x%02X) - failed: %d.", __func__, cmd_ch1, result);
    return result;
  }
  result = tsl2561_read(handle, data_ch1, 2);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_read() - failed: %d.", __func__, result);
    return result;
  }
  ESP_LOGD(TAG, "[%s] <- data: 0x%02X%02X", __func__, data_ch1[1], data_ch1[0]);
  handle->infrared = (data_ch1[1] << 8) | data_ch1[0];
  ESP_LOGD(TAG, "--> CH1: 0x%04X [%d]", handle->infrared, handle->infrared);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Set interrupt control registers
 * 
 * @param handle 
 * @param ctrl 
 *  - CTRL_ISR_CLEAR - clear interrupt
 *  - CTRL_ISR_DISABLE - disable interrupt
 *  - CTRL_ISR_ENABLE - enable interrupt
 * @return esp_err_t 
 */
static esp_err_t tsl2561_setIsrControl(const tsl2561_t handle, const tsl2561_ctrl_e ctrl) {
  uint8_t cmd = TSL2561_REG_COMMAND | TSL2561_REG_INTCTL;
  uint8_t data = 0;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, ctrl: %d)", __func__, handle, ctrl);

  switch (ctrl) {
    case CTRL_ISR_CLEAR: {
      cmd |= TSL2561_CMD_INTR_CLEAR;
      ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd);
      result = tsl2561_write(handle, &cmd, 1);
      break;
    }
    case CTRL_ISR_DISABLE:
    case CTRL_ISR_ENABLE: {
      if (ctrl == CTRL_ISR_ENABLE) {
        data = TSL2561_INTR_LEVEL | TSL2561_INTR_OUTSIDE_THRESHOLD;
        //data = TSL2561_INTR_LEVEL | TSL2561_INTR_EVERY_ADC;
      } else {
        data = TSL2561_INTR_DISABLE;
      }
      ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd);
      result = tsl2561_write(handle, &cmd, 1);
      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] tsl2561_write(cmd: 0x%02X) - failed: %d.", __func__, cmd, result);
        return result;
      }
      ESP_LOGD(TAG, "[%s] -> data: 0x%02X", __func__, data);
      result = tsl2561_write(handle, &data, 1);
      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] tsl2561_write(data: 0x%02X) - failed: %d.", __func__, data, result);
        return result;
      }
      break;
    }
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Set Interrupt threshold
 * 
 * @param handle 
 * @param threshold
 * - if NULL then set threshold to [0, 0]
 * - else set threshold to [min, max]
 * @return esp_err_t 
 */
static esp_err_t tsl2561_setThreshold(const tsl2561_t handle, const tsl2561_threshold_t* threshold) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, threshold: %p)", __func__, handle, threshold);
  if (threshold) {
    uint8_t cmd_min = TSL2561_REG_COMMAND | TSL2561_CMD_WORD_PROTOCOL | TSL2561_REG_THRESH_L;
    uint8_t cmd_max = TSL2561_REG_COMMAND | TSL2561_CMD_WORD_PROTOCOL | TSL2561_REG_THRESH_H;
    uint8_t data_min[2] = { threshold->min & 0x00FF, (threshold->min & 0xFF00) >> 8 };
    uint8_t data_max[2] = { threshold->max & 0x00FF, (threshold->max & 0xFF00) >> 8 };
    
    handle->threshold.min = threshold->min;
    handle->threshold.max = threshold->max;

    ESP_LOGE(TAG, "[%s] THRESHOLD ==> min: %d [%02X %02X], max: %d [%02X %02X]", __func__, 
      threshold->min, data_min[0], data_min[1],
      threshold->max, data_max[0], data_max[1]
    );

    /* Write threshold min & max to the Interrupt Threshold Register */
    ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd_min);
    result = tsl2561_write(handle, &cmd_min, 1);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] tsl2561_write(cmd_min: 0x%02X) - failed: %d.", __func__, cmd_min, result);
      return result;
    }
    ESP_LOGD(TAG, "[%s] -> data: 0x%02X%02X", __func__, data_min[1], data_min[0]);
    result = tsl2561_write(handle, data_min, 2);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] tsl2561_write(data_min: 0x%02X%02X) - failed: %d.", __func__, data_min[0], data_min[1], result);
      return result;
    }
    ESP_LOGD(TAG, "[%s] -> cmd: 0x%02X", __func__, cmd_max);
    result = tsl2561_write(handle, &cmd_max, 1);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] tsl2561_write(cmd_max: 0x%02X) - failed: %d.", __func__, cmd_max, result);
      return result;
    }
    ESP_LOGD(TAG, "[%s] -> data: 0x%02X%02X", __func__, data_max[1], data_max[0]);
    result = tsl2561_write(handle, data_max, 2);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] tsl2561_write(data_max: 0x%02X%02X) - failed: %d.", __func__, data_max[0], data_max[1], result);
      return result;
    }
  } else {
    handle->threshold.min = 0;
    handle->threshold.max = 0;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get Lux/CH0 factor depends on CH1/CH0 
 * 
 * @param handle 
 * @param ratio - ratio CH1/CH0
 * @param b - factor for CH0
 * @param m - factor for CH1
 * @return esp_err_t 
 */
static esp_err_t tsl2561_getFactor(const tsl2561_t handle, const uint32_t ratio, uint32_t* b, uint32_t* m) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, ratio: %f)", __func__, handle, handle->ratio);
  switch (handle->partno) {
    case PART_TSL2560CS:
    case PART_TSL2561CS: {

      if (ratio <= K1C) {
        *b = B1C;
        *m = M1C;
      } else if (ratio <= K2C) {
        *b = B2C;
        *m = M2C;
      } else if (ratio <= K3C) {
        *b = B3C;
        *m = M3C;
      } else if (ratio <= K4C) {
        *b = B4C;
        *m = M4C;
      } else if (ratio <= K5C) {
        *b = B5C;
        *m = M5C;
      } else if (ratio <= K6C) {
        *b = B6C;
        *m = M6C;
      } else if (ratio <= K7C) {
        *b = B7C;
        *m = M7C;
      } else if (ratio > K8C) {
        *b = B8C;
        *m = M8C;
      }       
      break;
    }
    case PART_TSL2560T_FN_CL:
    case PART_TSL2561T_FN_CL: {
      if (ratio <= K1T) {
        *b = B1T; 
        *m = M1T;
      } else if (ratio <= K2T) {
        *b = B2T;
        *m = M2T;
      } else if (ratio <= K3T) {
        *b = B3T;
        *m = M3T;
      } else if (ratio <= K4T) {
        *b = B4T;
        *m = M4T;
      } else if (ratio <= K5T) {
        *b = B5T;
        *m = M5T;
      } else if (ratio <= K6T) {
        *b = B6T;
        *m = M6T;
      } else if (ratio <= K7T) {
        *b = B7T;
        *m = M7T;
      } else if (ratio > K8T) {
        *b = B8T;
        *m = M8T;
      }
      break;
    }
    default: {
      ESP_LOGE(TAG, "[%s] Invalid Part Number Identification: 0x%02X ", __func__, handle->partno);
      return ESP_ERR_NOT_SUPPORTED;
    }
  }
  ESP_LOGD(TAG, "--> Factors: B: %ld, M: %ld", *b, *m);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Calculating Lux
 * 
 * @param handle 
 * @return esp_err_t 
 */
static esp_err_t tsl2561_calculateLux(const tsl2561_t handle) {
  uint32_t chScale;
  uint32_t channel0;
  uint32_t channel1;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);

  if (handle->broadband == 0) {
    ESP_LOGE(TAG, "[%s] Invalid Part Number Identification: 0x%02X ", __func__, handle->partno);
    return ESP_FAIL;
  }
  ESP_LOGD(TAG, "--> RATIO: %f [ch0: %d, ch1: %d]", handle->ratio, handle->broadband, handle->infrared);

  switch (handle->integ) {
    case INTEG_13MS: {
      chScale = CHSCALE_TINT0;
      break;
    }
    case INTEG_101MS: {
      chScale = CHSCALE_TINT1;
      break;
    }
    default: {
      chScale = (1 << CH_SCALE);
      break;
    }
  }
  ESP_LOGD(TAG, "--> chScale: 0x%08lX", chScale);

  // scale if gain is NOT 16X
  if (handle->gain == GAIN_1X) {
    chScale = chScale << 4; // scale 1X to 16X
  }

  // scale the channel values
  channel0 = (handle->broadband * chScale) >> CH_SCALE;
  channel1 = (handle->infrared * chScale) >> CH_SCALE;  

  // find the ratio of the channel values (Channel1/Channel0)
  // protect against divide by zero
  uint32_t ratio = 0;
  if (channel0 != 0) {
    ratio = (channel1 << (RATIO_SCALE + 1)) / channel0;
  }
  // round the ratio value
  ratio = (ratio + 1) >> 1;

  uint32_t b = 0;
  uint32_t m = 0;

  result = tsl2561_getFactor(handle, ratio, &b, &m);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_getFactor() - failed: %d", __func__, result);
    return result;
  }

  uint32_t lux = ((channel0 * b) - (channel1 * m));

  // round lsb (2^(LUX_SCALE−1))
  lux += (1 << (LUX_SCALE - 1));

  // strip off fractional portion
  handle->lux = lux >> LUX_SCALE;

  /* protect against divade by 0 */
  if (handle->broadband != 0) {
    handle->ratio = (float) handle->infrared / (float) handle->broadband;
  } else {
    handle->ratio = 0;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initialize I2C bus and add device to the bus
 * 
 * @return esp_err_t 
 */
static esp_err_t tsl2561_init(tsl2561_t* const handle_ptr) {
  tsl2561_t handle = NULL;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle_ptr: %p)", __func__, handle_ptr);

  if (handle_ptr == NULL) {
    ESP_LOGE(TAG, "[%s] handle_ptr=NULL", __func__);
    return ESP_FAIL;
  }

  handle = malloc(sizeof(tsl2561_s));
  if (handle == NULL) {
    ESP_LOGE(TAG, "[%s] Memory allocation problem", __func__);
    result = ESP_ERR_NO_MEM;
  }

  memset(handle, 0x00, sizeof(tsl2561_s));

  ESP_LOGD(TAG, "Initialize I2C bus");
  result = i2c_new_master_bus(&tsl2561_bus_config, &(handle->bus));
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_new_master_bus() failed: %d.", __func__, result);
    free(handle);
    return result;
  }

  ESP_ERROR_CHECK(i2c_master_probe(handle->bus, TSL2561_SLAVE_ADDR, -1));

  ESP_LOGD(TAG, "Add device to the bus");
  result = i2c_master_bus_add_device(handle->bus, &tsl2561_dev_cfg, &(handle->device));

  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] i2c_master_bus_add_device() failed: %d.", __func__, result);
    free(handle);
    return result;
  }

  tsl2561_setPower(handle, true);

  result = tsl2561_readId(handle);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_readId() - result: %d.", __func__, result);
    return result;
  }

  result = tsl2561_readTiming(handle);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_readTiming() - result: %d.", __func__, result);
    return result;
  }

  //tsl2561_setPower(handle, false);

  *handle_ptr = handle;
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Remove device from the bus and delete bus
 * 
 * @return esp_err_t 
 */
static esp_err_t tsl2561_done(tsl2561_t handle) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);

  if (handle->device) {
    result = i2c_master_bus_rm_device(handle->device);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] i2c_master_bus_rm_device() failed: %d.", __func__, result);
    }
  }

  if (handle->bus != NULL) {
    result = i2c_del_master_bus(handle->bus);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] i2c_del_master_bus() failed: %d.", __func__, result);
    }
  }

  free(handle);
  handle = NULL;

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}


/* ================================================================================== */

/**
 * @brief Set Power On/Off
 * 
 * @param handle 
 * @param on 
 * @return esp_err_t 
 */
esp_err_t tsl2561_SetPower(const tsl2561_t handle, const bool on) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  result = tsl2561_setPower(handle, on);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get Power state (On/Off)
 * 
 * @param handle 
 * @return esp_err_t 
 */
esp_err_t tsl2561_GetPower(const tsl2561_t handle, bool* on) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  result = tsl2561_getPower(handle, on);
  ESP_LOGI(TAG, "--%s(on: %d) - result: %d", __func__, *on, result);
  return result;
}

/**
 * @brief Set Gain
 * 
 * @param handle 
 * @param gain 
 * @return esp_err_t 
 */
esp_err_t tsl2561_SetGain(const tsl2561_t handle, const tsl2561_gain_e gain) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, gain: 0x%02X)", __func__, handle, gain);
  result = tsl2561_writeGain(handle, gain);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_writeGain() - failed: %d.", __func__, result);
    return result;
  }
  result = tsl2561_readTiming(handle);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Set Integration Time
 * 
 * @param handle 
 * @param time 
 * @return esp_err_t 
 */
esp_err_t tsl2561_SetIntegrationTime(const tsl2561_t handle, const tsl2561_integ_e time) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, time: %d)", __func__, handle, time);
  result = tsl2561_writeIntegrationTime(handle, time);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_writeIntegrationTime() - failed: %d.", __func__, result);
    return result;
  }
  result = tsl2561_readTiming(handle);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get current lux
 * 
 * @param handle 
 * @param lux
 * @return esp_err_t 
 */
esp_err_t tsl2561_GetLux(const tsl2561_t handle, uint32_t* lux) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  //tsl2561_setPower(handle, true);
  result = tsl2561_readChannels(handle);
  //tsl2561_setPower(handle, false);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_readChannels() - result: %d.", __func__, result);
    return result;
  }
  result = tsl2561_calculateLux(handle);
  if (result == ESP_OK) {
    *lux = handle->lux;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get light parameters
 * 
 * @param handle 
 * @param broadband
 * @param infrared 
 * @param lux 
 * @param ratio 
 * @return esp_err_t 
 */
esp_err_t tsl2561_GetLight(const tsl2561_t handle, uint16_t* broadband, uint16_t* infrared, uint32_t* lux, float* ratio) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  //tsl2561_setPower(handle, true);
  result = tsl2561_readChannels(handle);
  //tsl2561_setPower(handle, false);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_readChannels() - result: %d.", __func__, result);
    return result;
  }
  result = tsl2561_calculateLux(handle);
  if (result == ESP_OK) {
    *broadband  = handle->broadband;
    *infrared   = handle->infrared;
    *lux        = handle->lux;
    *ratio      = handle->ratio;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Set Interrupt's notify function
 *        When the current light level is outside the threshold range, 
 *        an interrupt will occur and the set fn will be called.
 * @param handle 
 * @param fn 
 * @return esp_err_t 
 */
esp_err_t tsl2561_SetIsrNotify(const tsl2561_t handle, tsl2561_isr_f fn) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  result = tsl2561_setIsrNotify(handle, fn);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Set Interrupt Threshold values (min/max)
 * 
 * @param handle 
 * @param threshold struct
 * @return esp_err_t 
 */
esp_err_t tsl2561_SetIsrThreshold(const tsl2561_t handle, const tsl2561_threshold_t* threshold) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, threshold: %p)", __func__, handle, threshold);
  if (threshold) {
    ESP_LOGD(TAG, "Enable threshold -> min: %d, max: %d", threshold->min, threshold->max);
  } else {
    ESP_LOGD(TAG, "Disable threshold");
  }
  result = tsl2561_setThreshold(handle, threshold);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Set interrupt control registers
 * 
 * @param handle 
 * @param ctrl 
 *  - CTRL_ISR_CLEAR - clear interrupt
 *  - CTRL_ISR_DISABLE - disable interrupt
 *  - CTRL_ISR_ENABLE - enable interrupt
 * @return esp_err_t 
 */
esp_err_t tsl2561_SetIsrControl(const tsl2561_t handle, const tsl2561_ctrl_e ctrl) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p, ctrl: %d)", __func__, handle, ctrl);
  result = tsl2561_setIsrControl(handle, ctrl);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init Interrupt configuration
 * 
 * @param handle 
 * @return esp_err_t 
 */
esp_err_t tsl2561_InitIsr(const tsl2561_t handle) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  result = tsl2561_setIsrConfig(handle, true);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done Interrupt configuration
 * 
 * @param handle 
 * @return esp_err_t 
 */
esp_err_t tsl2561_DoneIsr(const tsl2561_t handle) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  result = tsl2561_setIsrConfig(handle, false);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Get ID (PARTNO & REVNO)
 * 
 * @param handle 
 * @param id
 * @return esp_err_t 
 */
esp_err_t tsl2561_GetId(const tsl2561_t handle, uint8_t *id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  result = tsl2561_readId(handle);
  if (result == ESP_OK) {
    *id = handle->partno | handle->revno;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initialize TSL2561 driver
 * 
 * @return esp_err_t 
 */
esp_err_t tsl2561_Init(tsl2561_t* const handle_ptr) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_TSL2561_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s(handle_ptr: %p)", __func__, handle_ptr);
  if (handle_ptr != NULL) {
    result = tsl2561_init(handle_ptr);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] tsl2561_init() - result: %d.", __func__, result);
      return result;
    }

    tsl2561_print(*handle_ptr);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t tsl2561_Done(tsl2561_t handle) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(handle: %p)", __func__, handle);
  result = tsl2561_done(handle);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
