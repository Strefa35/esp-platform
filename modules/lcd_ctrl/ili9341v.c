/**
 * @file ili9341v.c
 * @author A.Czerwinski@pistacje.net
 * @brief TFT LCD Single Chip Driver
 * @version 0.1
 * @date 2025-02-22
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "sdkconfig.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"

#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "ili9341v.h"


#define LCD_HOST                   SPI2_HOST

#define LCD_PIXEL_CLOCK_HZ         (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL      1

#define LCD_PIN_NUM_SCLK           14
#define LCD_PIN_NUM_MOSI           2
#define LCD_PIN_NUM_MISO           -1
#define LCD_PIN_NUM_LCD_DC         15
#define LCD_PIN_NUM_LCD_RST        -1
#define LCD_PIN_NUM_LCD_CS         17
#define LCD_PIN_NUM_BK_LIGHT       4

#define LCD_BK_LIGHT_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define LCD_BK_LIGHT_LEDC_TIMER        LEDC_TIMER_0
#define LCD_BK_LIGHT_LEDC_CHANNEL      LEDC_CHANNEL_0
#define LCD_BK_LIGHT_LEDC_FREQ_HZ      5000
#define LCD_BK_LIGHT_LEDC_DUTY_RES     LEDC_TIMER_13_BIT
#define LCD_BK_LIGHT_LEDC_MAX_DUTY     ((1U << 13) - 1U)

#define LCD_H_RES                  320
#define LCD_V_RES                  240

#define LCD_CMD_BITS               8
#define LCD_PARAM_BITS             8


static const char* TAG = "ESP::LCD::ILI9341V";

static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static bool s_spi_inited = false;
static bool s_backlight_inited = false;
static lcd_flush_done_cb_t s_flush_done_cb = NULL;
static void* s_flush_done_ctx = NULL;

/**
 * @brief Convert 0–100% brightness to LEDC duty for the configured timer resolution.
 *
 * @param percent Input brightness; values above 100 are clamped.
 * @return Duty cycle units for `ledc_set_duty`.
 */
static uint32_t lcd_BacklightPercentToDuty(uint8_t percent) {
  if (percent > 100U) {
    percent = 100U;
  }
  return (LCD_BK_LIGHT_LEDC_MAX_DUTY * percent) / 100U;
}

/**
 * @brief Drive the panel backlight GPIO via LEDC PWM.
 *
 * Requires successful display init (backlight channel configured).
 *
 * @param percent Brightness 0–100.
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if backlight was not initialized.
 */
esp_err_t lcd_SetBacklightPercent(uint8_t percent) {
  if (!s_backlight_inited) {
    return ESP_ERR_INVALID_STATE;
  }

  uint32_t duty = lcd_BacklightPercentToDuty(percent);
  if (LCD_BK_LIGHT_ON_LEVEL == 0) {
    duty = LCD_BK_LIGHT_LEDC_MAX_DUTY - duty;
  }

  ESP_RETURN_ON_ERROR(
    ledc_set_duty(LCD_BK_LIGHT_LEDC_MODE, LCD_BK_LIGHT_LEDC_CHANNEL, duty),
    TAG,
    "ledc_set_duty failed");
  ESP_RETURN_ON_ERROR(
    ledc_update_duty(LCD_BK_LIGHT_LEDC_MODE, LCD_BK_LIGHT_LEDC_CHANNEL),
    TAG,
    "ledc_update_duty failed");
  return ESP_OK;
}


/**
 * @brief Handles panel I/O transfer-done event.
 *
 * @param panel_io Panel I/O handle (unused).
 * @param edata Event payload (unused).
 * @param user_ctx User context (unused).
 * @return true If higher priority task should be woken.
 * @return false Otherwise.
 */
static bool lcd_OnColorTransDone(esp_lcd_panel_io_handle_t panel_io,
                                 esp_lcd_panel_io_event_data_t* edata,
                                 void* user_ctx) {
  (void) panel_io;
  (void) edata;
  (void) user_ctx;

  if (s_flush_done_cb != NULL) {
    s_flush_done_cb(s_flush_done_ctx);
  }
  return false;
}

/**
 * @brief Configures SPI bus, LCD panel and DMA frame buffers.
 *
 * @param lcd_ptr LCD context structure to initialize.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t lcd_SetupDisplayHw(lcd_t* lcd_ptr) {
  ESP_LOGI(TAG, "++%s()", __func__);

  lcd_ptr->h_res = LCD_H_RES;
  lcd_ptr->v_res = LCD_V_RES;

  ledc_timer_config_t ledc_timer = {
    .speed_mode = LCD_BK_LIGHT_LEDC_MODE,
    .duty_resolution = LCD_BK_LIGHT_LEDC_DUTY_RES,
    .timer_num = LCD_BK_LIGHT_LEDC_TIMER,
    .freq_hz = LCD_BK_LIGHT_LEDC_FREQ_HZ,
    .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_RETURN_ON_ERROR(ledc_timer_config(&ledc_timer), TAG, "ledc_timer_config failed");

  ledc_channel_config_t ledc_channel = {
    .gpio_num = LCD_PIN_NUM_BK_LIGHT,
    .speed_mode = LCD_BK_LIGHT_LEDC_MODE,
    .channel = LCD_BK_LIGHT_LEDC_CHANNEL,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LCD_BK_LIGHT_LEDC_TIMER,
    .duty = 0,
    .hpoint = 0,
  };
  ESP_RETURN_ON_ERROR(ledc_channel_config(&ledc_channel), TAG, "ledc_channel_config failed");
  s_backlight_inited = true;
  ESP_RETURN_ON_ERROR(lcd_SetBacklightPercent(0), TAG, "backlight set failed");

  spi_bus_config_t buscfg = {
    .sclk_io_num = LCD_PIN_NUM_SCLK,
    .mosi_io_num = LCD_PIN_NUM_MOSI,
    .miso_io_num = LCD_PIN_NUM_MISO,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = lcd_ptr->h_res * 40 * sizeof(uint16_t),
  };
  esp_err_t result = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (result == ESP_OK) {
    s_spi_inited = true;
  } else if (result == ESP_ERR_INVALID_STATE) {
    result = ESP_OK;
  }
  ESP_RETURN_ON_ERROR(result, TAG, "spi_bus_initialize failed");

  esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = LCD_PIN_NUM_LCD_DC,
    .cs_gpio_num = LCD_PIN_NUM_LCD_CS,
    .pclk_hz = LCD_PIXEL_CLOCK_HZ,
    .lcd_cmd_bits = LCD_CMD_BITS,
    .lcd_param_bits = LCD_PARAM_BITS,
    .spi_mode = 0,
    .trans_queue_depth = 10,
  };
  ESP_RETURN_ON_ERROR(
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) LCD_HOST, &io_config, &s_io_handle),
    TAG,
    "esp_lcd_new_panel_io_spi failed");

  const esp_lcd_panel_io_callbacks_t io_callbacks = {
    .on_color_trans_done = lcd_OnColorTransDone,
  };
  ESP_RETURN_ON_ERROR(
    esp_lcd_panel_io_register_event_callbacks(s_io_handle, &io_callbacks, NULL),
    TAG,
    "panel io callback registration failed");

  esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = LCD_PIN_NUM_LCD_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
    .bits_per_pixel = 16,
  };

  ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ili9341(s_io_handle, &panel_config, &s_panel_handle), TAG, "new panel failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel_handle), TAG, "panel reset failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel_handle), TAG, "panel init failed");

  ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(s_panel_handle, true), TAG, "panel swap_xy failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(s_panel_handle, false, false), TAG, "panel mirror failed");
  ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel_handle, true), TAG, "panel on failed");

  ESP_RETURN_ON_ERROR(lcd_SetBacklightPercent(100), TAG, "backlight set failed");

  lcd_ptr->buffer_size = (size_t) ((lcd_ptr->h_res * lcd_ptr->v_res) / 10U) * sizeof(uint16_t);
  lcd_ptr->buffer1 = spi_bus_dma_memory_alloc(LCD_HOST, lcd_ptr->buffer_size, 0);
  lcd_ptr->buffer2 = spi_bus_dma_memory_alloc(LCD_HOST, lcd_ptr->buffer_size, 0);
  assert(lcd_ptr->buffer1 != NULL);
  assert(lcd_ptr->buffer2 != NULL);

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}

/**
 * @brief Initializes LCD display hardware driver.
 *
 * @param lcd_ptr LCD context structure to initialize.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t lcd_InitDisplayHw(lcd_t* lcd_ptr) {
  esp_log_level_set(TAG, CONFIG_LCD_ILI9341V_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  esp_err_t result = lcd_SetupDisplayHw(lcd_ptr);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Deinitializes LCD display hardware driver.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t lcd_DoneDisplayHw(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  if (s_panel_handle != NULL) {
    if (s_backlight_inited) {
      lcd_SetBacklightPercent(0);
    }
    esp_lcd_panel_del(s_panel_handle);
    s_panel_handle = NULL;
  }

  if (s_io_handle != NULL) {
    esp_lcd_panel_io_del(s_io_handle);
    s_io_handle = NULL;
  }

  s_flush_done_cb = NULL;
  s_flush_done_ctx = NULL;

  if (s_backlight_inited) {
    ledc_stop(LCD_BK_LIGHT_LEDC_MODE, LCD_BK_LIGHT_LEDC_CHANNEL, !LCD_BK_LIGHT_ON_LEVEL);
    s_backlight_inited = false;
  }

  if (s_spi_inited) {
    result = spi_bus_free(LCD_HOST);
    s_spi_inited = false;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Flushes a rectangular area to LCD panel.
 *
 * @param x1 Left coordinate.
 * @param y1 Top coordinate.
 * @param x2 Right coordinate.
 * @param y2 Bottom coordinate.
 * @param color_map Pointer to RGB565 pixel data.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t lcd_FlushDisplayArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const void* color_map) {
  if (s_panel_handle == NULL || color_map == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  return esp_lcd_panel_draw_bitmap(s_panel_handle, x1, y1, x2 + 1, y2 + 1, color_map);
}

/**
 * @brief Registers callback invoked after display flush is finished.
 *
 * @param cb Callback function pointer.
 * @param ctx User context passed to callback.
 * @return esp_err_t Always ESP_OK.
 */
esp_err_t lcd_SetFlushDoneCallback(lcd_flush_done_cb_t cb, void* ctx) {
  s_flush_done_cb = cb;
  s_flush_done_ctx = ctx;
  return ESP_OK;
}
