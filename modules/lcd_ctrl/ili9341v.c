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
#include <stdio.h>
#include <stdbool.h>

#include "sdkconfig.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "esp_log.h"
#include "esp_err.h"

#include "ili9341v.h"

#include "esp_lcd_ili9341.h"


// Using SPI2 in the example
#define LCD_HOST                  SPI2_HOST

#define LCD_PIXEL_CLOCK_HZ        (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL     1
#define LCD_BK_LIGHT_OFF_LEVEL    !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL

#define LCD_PIN_NUM_SCLK          14
#define LCD_PIN_NUM_MOSI          2
#define LCD_PIN_NUM_MISO          -1
#define LCD_PIN_NUM_LCD_DC        15
#define LCD_PIN_NUM_LCD_RST       -1
#define LCD_PIN_NUM_LCD_CS        17
#define LCD_PIN_NUM_BK_LIGHT      4

// The pixel number in horizontal and vertical
#define LCD_H_RES                 240
#define LCD_V_RES                 320

// Bit number used to represent command and parameter
#define LCD_CMD_BITS              8
#define LCD_PARAM_BITS            8


static const char* TAG = "ESP::LCD::ILI9341V";

static esp_err_t lcd_SetupDisplayHw(lcd_t* lcd_ptr) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  lcd_ptr->h_res = LCD_H_RES;
  lcd_ptr->v_res = LCD_V_RES;

  ESP_LOGI(TAG, "Turn off LCD backlight");
  gpio_config_t bk_gpio_config = {
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = 1ULL << LCD_PIN_NUM_BK_LIGHT
  };
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

  ESP_LOGI(TAG, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .sclk_io_num = LCD_PIN_NUM_SCLK,
      .mosi_io_num = LCD_PIN_NUM_MOSI,
      .miso_io_num = LCD_PIN_NUM_MISO,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = lcd_ptr->h_res * 80 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num = LCD_PIN_NUM_LCD_DC,
      .cs_gpio_num = LCD_PIN_NUM_LCD_CS,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .lcd_cmd_bits = LCD_CMD_BITS,
      .lcd_param_bits = LCD_PARAM_BITS,
      .spi_mode = 0,
      .trans_queue_depth = 10,
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = LCD_PIN_NUM_LCD_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
      .bits_per_pixel = 16,
  };

  ESP_LOGI(TAG, "Install ILI9341 panel driver");
  ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

  // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  ESP_LOGI(TAG, "Turn on LCD backlight");
  gpio_set_level(LCD_PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);

  // Allocate draw 2 buffers for LVGL
  // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
  lcd_ptr->buffer_size = (lcd_ptr->h_res * lcd_ptr->v_res) >> 3;

  lcd_ptr->buffer1 = spi_bus_dma_memory_alloc(LCD_HOST, lcd_ptr->buffer_size, 0);
  assert(lcd_ptr->buffer1);
  lcd_ptr->buffer2 = spi_bus_dma_memory_alloc(LCD_HOST, lcd_ptr->buffer_size, 0);
  assert(lcd_ptr->buffer2);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t lcd_InitDisplayHw(lcd_t* lcd_ptr) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_LCD_ILI9341V_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = lcd_SetupDisplayHw(lcd_ptr);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
