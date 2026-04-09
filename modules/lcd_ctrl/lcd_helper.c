/**
 * @file lcd_helper.c
 * @author A.Czerwinski@pistacje.net
 * @brief LCD Helper
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sdkconfig.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_ip_addr.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "ili9341v.h"
#include "lcd_defs.h"
#include "lcd_helper.h"
#include "lcd_hw.h"
#include "lvgl.h"
#include "ns2009.h"
#include "ui_main_screen.h"
#include "ui_screensaver.h"

/** Set to 1 to restore idle timeout and screensaver (ui_screensaver.c kept in tree). */
#define LCD_SCREENSAVER_IDLE_ENABLE 0

#define LCD_UI_TASK_NAME             "lcd-ui"
/* LVGL main + complex flex UI + timer callbacks need more than 6 KB on Xtensa. */
#define LCD_UI_TASK_STACK_SIZE       (16 * 1024)
#define LCD_UI_TASK_PRIORITY         9

#define LCD_LVGL_TICK_MS             2
#define LCD_LVGL_HANDLER_MS          10
#define LCD_SCREENSAVER_IDLE_MS      (15 * 1000) /* 15 seconds of inactivity before switching to screensaver */
#define LCD_BRIGHTNESS_MAIN_PCT      100
#define LCD_BRIGHTNESS_SAVER_PCT     10
#define LCD_TOUCH_STABLE_SAMPLES     3

typedef struct {
  uint32_t update; /* Last lcd_UpdateData() mask argument */
  uint32_t mask;   /* Accumulated mask (fields ever updated) */

  bool     eth_connected;
  bool     wifi_connected;
  bool     mqtt_connected;
  bool     bt_connected;
  uint32_t ambient_lux;
  uint32_t ambient_threshold_lux;
} lcd_data_t;

typedef struct {
  char uid[32];
  char mac[24];
  char ip[24];
} lcd_runtime_data_t;


static lcd_t s_lcd = {
  .h_res = 0,
  .v_res = 0,
  .buffer_size = 0,
  .buffer1 = NULL,
  .buffer2 = NULL,
};

static lcd_runtime_data_t s_runtime = {
  .uid = "ESP/------",
  .mac = "--:--:--:--:--:--",
  .ip = "0.0.0.0",
};

static const char* TAG = "ESP::LCD::HELPER";

static SemaphoreHandle_t s_state_mutex = NULL;
static TaskHandle_t s_ui_task = NULL;
static esp_timer_handle_t s_tick_timer = NULL;

static lv_display_t* s_display = NULL;
static lv_indev_t* s_touch_indev = NULL;

typedef enum {
  LCD_SCREEN_MAIN = 0,
  LCD_SCREEN_SCREENSAVER,
} lcd_screen_t;

static lcd_screen_t s_active_screen = LCD_SCREEN_SCREENSAVER;
static int64_t s_last_touch_us = 0;
static bool s_touch_was_pressed = false;
static uint8_t s_touch_press_stable_cnt = 0;
static int32_t s_last_idle_log_s = -1;

/* Aggregated UI state (connection flags, ambient): lcd_UpdateData writes, LVGL timer reads */
static lcd_data_t s_data = {
  .update = 0,
  .mask = 0,
  .eth_connected = false,
  .wifi_connected = false,
  .mqtt_connected = false,
  .bt_connected = false,
  .ambient_lux = 0,
  .ambient_threshold_lux = 500,
};

/**
 * @brief Set panel backlight brightness via the hardware layer (logs on failure).
 *
 * @param percent Brightness 0–100.
 */
static void lcd_set_backlight(uint8_t percent) {
  esp_err_t result = lcd_SetBacklightPercent(percent);
  if (result != ESP_OK) {
    ESP_LOGW(TAG, "[%s] lcd_SetBacklightPercent(%u) failed: %d", __func__, percent, result);
  }
}

/**
 * @brief Switch between main UI and screensaver; recreates the target screen and adjusts backlight.
 *
 * @param screen Target screen (`LCD_SCREEN_MAIN` or `LCD_SCREEN_SCREENSAVER`).
 */
static void lcd_switch_screen(lcd_screen_t screen) {
  if (s_display == NULL || s_active_screen == screen) {
    return;
  }

  lcd_screen_t prev_screen = s_active_screen;

  if (screen == LCD_SCREEN_MAIN) {
    ui_main_screen_create(s_display, NULL);
    lcd_set_backlight(LCD_BRIGHTNESS_MAIN_PCT);
  } else {
    ui_screensaver_create(s_display);
    lcd_set_backlight(LCD_BRIGHTNESS_SAVER_PCT);
  }

  s_active_screen = screen;
  ESP_LOGD(TAG, "[%s] Screen changed: %s -> %s", __func__,
           (prev_screen == LCD_SCREEN_MAIN) ? "main" : "screensaver",
           (screen == LCD_SCREEN_MAIN) ? "main" : "screensaver");
}


/**
 * @brief Increments LVGL internal tick counter.
 *
 * @param arg Unused callback argument.
 */
static void lcd_lvgl_tick_cb(void* arg) {
  (void) arg;
  lv_tick_inc(LCD_LVGL_TICK_MS);
}

/**
 * @brief Flushes rendered LVGL area to LCD hardware.
 *
 * @param display LVGL display instance.
 * @param area Area to refresh.
 * @param px_map Pixel buffer in RGB565 format.
 */
static void lcd_lvgl_flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
  uint32_t width = (uint32_t) (area->x2 - area->x1 + 1);
  uint32_t height = (uint32_t) (area->y2 - area->y1 + 1);

  /* SPI panels usually expect RGB565 bytes in opposite order than CPU memory layout. */
  lv_draw_sw_rgb565_swap(px_map, width * height);

  esp_err_t result = lcd_FlushDisplayArea(area->x1, area->y1, area->x2, area->y2, px_map);
  if (result != ESP_OK) {
    lv_display_flush_ready(display);
  }
}

/**
 * @brief Signals LVGL that display flushing has finished.
 *
 * @param ctx LVGL display pointer passed as callback context.
 */
static void lcd_lvgl_flush_done_cb(void* ctx) {
  lv_display_t* display = (lv_display_t*) ctx;
  lv_display_flush_ready(display);
}

/**
 * @brief Reads current touch state and maps it to LVGL input data.
 *
 * @param indev LVGL input device (unused).
 * @param data LVGL input data to fill.
 */
static void lcd_lvgl_touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
  (void) indev;

  ns2009_touch_t touch = {0, 0, 0};
  bool touch_raw_pressed = (ns2009_GetTouch(&touch) == ESP_OK);
  if (touch_raw_pressed) {
    if (s_touch_press_stable_cnt < LCD_TOUCH_STABLE_SAMPLES) {
      s_touch_press_stable_cnt++;
    }

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = touch.x;
    data->point.y = touch.y;

    if (!s_touch_was_pressed && s_touch_press_stable_cnt >= LCD_TOUCH_STABLE_SAMPLES) {
      s_touch_was_pressed = true;
      s_last_touch_us = esp_timer_get_time();
      ESP_LOGD(TAG, "[%s] Valid touch detected (stable=%u), idle timer reset", __func__, s_touch_press_stable_cnt);
      if (s_active_screen == LCD_SCREEN_SCREENSAVER) {
        lcd_switch_screen(LCD_SCREEN_MAIN);
      }
    }
  }
  else {
    s_touch_press_stable_cnt = 0;
    s_touch_was_pressed = false;
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/**
 * @brief Periodically refreshes UI labels with time and runtime data.
 *
 * @param timer LVGL timer instance (unused).
 */
static void lcd_ui_update_timer_cb(lv_timer_t* timer) {
  (void) timer;

#if LCD_SCREENSAVER_IDLE_ENABLE
  int64_t now_us = esp_timer_get_time();
  int64_t idle_us = now_us - s_last_touch_us;
  int32_t idle_s = (int32_t) (idle_us / 1000000LL);
  int32_t timeout_s = (int32_t) (LCD_SCREENSAVER_IDLE_MS / 1000);
  if (s_active_screen == LCD_SCREEN_MAIN && idle_s != s_last_idle_log_s && (idle_s % 5) == 0) {
    int32_t remain_s = timeout_s - idle_s;
    if (remain_s < 0) {
      remain_s = 0;
    }
    ESP_LOGD(TAG, "[%s] idle=%ds, remain=%ds", __func__, idle_s, remain_s);
    s_last_idle_log_s = idle_s;
  }
  if (idle_us >= ((int64_t) LCD_SCREENSAVER_IDLE_MS * 1000LL)) {
    ESP_LOGD(TAG, "[%s] Inactivity timeout reached (idle=%ds) -> screensaver", __func__, idle_s);
    lcd_switch_screen(LCD_SCREEN_SCREENSAVER);
  }
#endif

  /* Format current time and date strings */
  char time_str[16] = {0};
  char date_str[32] = {0};
  time_t now = time(NULL);
  struct tm local_tm = {0};
  localtime_r(&now, &local_tm);
  strftime(time_str, sizeof(time_str), "%H:%M:%S", &local_tm);
  strftime(date_str, sizeof(date_str), "%d.%m.%Y", &local_tm);

  /* Snapshot shared UI data (connections + ambient); ETH link from lcd_UpdateData(LCD_MASK_ETH_CONNECTED) */
  bool eth = false;
  bool wifi = false;
  bool mqtt = false;
  bool bt = false;
  uint32_t lux = 0;
  uint32_t th = 500;
  if (s_state_mutex != NULL && xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    eth   = s_data.eth_connected;
    wifi  = s_data.wifi_connected;
    mqtt  = s_data.mqtt_connected;
    bt    = s_data.bt_connected;
    lux   = s_data.ambient_lux;
    th    = s_data.ambient_threshold_lux;
    xSemaphoreGive(s_state_mutex);
  }

  if (s_active_screen == LCD_SCREEN_MAIN) {
    ui_main_screen_update_time(time_str, date_str);
    ui_main_screen_update_eth(eth);
    ui_main_screen_update_wifi(wifi);
    ui_main_screen_update_mqtt(mqtt);
    ui_main_screen_update_bluetooth(bt);
    ui_main_screen_update_ambient_lux(lux, th, 0);
  } else {
    ui_screensaver_update_time(time_str, date_str);
  }
}

/**
 * @brief Creates and initializes the main LCD UI screen.
 *
 * @param display LVGL display instance.
 */
static void lcd_create_ui(lv_display_t* display) {
  (void) display;

  s_last_touch_us = esp_timer_get_time();
  s_touch_was_pressed = false;
  s_touch_press_stable_cnt = 0;
  s_last_idle_log_s = -1;

  /* Must not set s_active_screen to MAIN here: lcd_switch_screen() no-ops when
   * already on the target screen, so the UI would never be created. */
  lcd_switch_screen(LCD_SCREEN_MAIN);

  lv_timer_create(lcd_ui_update_timer_cb, 1000, NULL);
  lcd_ui_update_timer_cb(NULL);
}

/**
 * @brief Runs LVGL handler loop in dedicated FreeRTOS task.
 *
 * @param param Unused task argument.
 */
static void lcd_lvgl_task(void* param) {
  (void) param;
  ESP_LOGI(TAG, "++%s()", __func__);

  while (true) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(LCD_LVGL_HANDLER_MS));
  }
}


/**
 * @brief Initializes LVGL display, touch input and UI task.
 *
 * @param lcd_ptr LCD runtime context with resolution and frame buffers.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t lcd_InitLvgl(lcd_t* lcd_ptr) {
  ESP_LOGI(TAG, "++%s()", __func__);

  lv_init();

  s_display = lv_display_create(lcd_ptr->h_res, lcd_ptr->v_res);
  if (s_display == NULL) {
    return ESP_FAIL;
  }

  /**
   * @brief Sets the color format for the LVGL display to RGB565.
   * 
   * This function configures the display to use the RGB565 color format,
   * which is a 16-bit color representation where:
   * - 5 bits for red channel
   * - 6 bits for green channel
   * - 5 bits for blue channel
   * 
   * RGB565 is commonly used in embedded systems and LCD displays because:
   * - It provides a good balance between color depth and memory usage
   * - It requires less memory than RGB888 (24-bit)
   * - It's supported by most LCD controllers and display hardware
   * - It's suitable for displaying graphics and UI elements on IoT devices
   * 
   * @param s_display Pointer to the LVGL display object to configure
   * @param LV_COLOR_FORMAT_RGB565 The RGB565 color format constant
   */
  lv_display_set_color_format(s_display, LV_COLOR_FORMAT_RGB565);

  lv_display_set_buffers(s_display, lcd_ptr->buffer1, lcd_ptr->buffer2, lcd_ptr->buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
  
  lv_display_set_flush_cb(s_display, lcd_lvgl_flush_cb);
  
  ESP_RETURN_ON_ERROR(lcd_SetFlushDoneCallback(lcd_lvgl_flush_done_cb, s_display), TAG, "lcd_SetFlushDoneCallback failed");

#if CONFIG_LCD_UI_ROTATION_90
  lv_display_set_rotation(s_display, LV_DISPLAY_ROTATION_90);
#elif CONFIG_LCD_UI_ROTATION_180
  lv_display_set_rotation(s_display, LV_DISPLAY_ROTATION_180);
#elif CONFIG_LCD_UI_ROTATION_270
  lv_display_set_rotation(s_display, LV_DISPLAY_ROTATION_270);
#else
  lv_display_set_rotation(s_display, LV_DISPLAY_ROTATION_0);
#endif

  s_touch_indev = lv_indev_create();
  if (s_touch_indev == NULL) {
    return ESP_FAIL;
  }
  lv_indev_set_type(s_touch_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(s_touch_indev, lcd_lvgl_touch_read_cb);

  const esp_timer_create_args_t tick_timer_args = {
    .callback = lcd_lvgl_tick_cb,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "lcd-lvgl-tick",
    .skip_unhandled_events = true,
  };
  ESP_RETURN_ON_ERROR(esp_timer_create(&tick_timer_args, &s_tick_timer), TAG, "esp_timer_create failed");
  ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_tick_timer, LCD_LVGL_TICK_MS * 1000), TAG, "esp_timer_start_periodic failed");

  lcd_create_ui(s_display);

  BaseType_t ok = xTaskCreate(lcd_lvgl_task, LCD_UI_TASK_NAME, LCD_UI_TASK_STACK_SIZE, NULL, LCD_UI_TASK_PRIORITY, &s_ui_task);
  if (ok != pdPASS) {
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}

/**
 * @brief Apply batched UI field updates under the state mutex (safe from the message task).
 *
 * For connection masks, set `d_bool[0]…[3]` in mask bit order (ETH, Wi-Fi, MQTT, BT).
 * UID, MAC, IP, and ambient fields: see `lcd_update_t` and `LCD_MASK_*` in lcd_helper.h.
 *
 * @param mask   OR of `LCD_MASK_*` bits selecting which fields in @p update are valid.
 * @param update Snapshot of values to merge into shared UI state.
 */
void lcd_UpdateData(const uint32_t mask, const lcd_update_t* update) {
  if (s_state_mutex == NULL || update == NULL) {
    return;
  }
  if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    if (mask & LCD_MASK_ETH_CONNECTED) {
      s_data.eth_connected = update->u.d_bool[0];
    }
    if (mask & LCD_MASK_WIFI_CONNECTED) {
      s_data.wifi_connected = update->u.d_bool[1];
    }
    if (mask & LCD_MASK_MQTT_CONNECTED) {
      s_data.mqtt_connected = update->u.d_bool[2];
    }
    if (mask & LCD_MASK_BT_CONNECTED) {
      s_data.bt_connected = update->u.d_bool[3];
    }

    if (mask & LCD_MASK_AMBIENT_LUX) {
      s_data.ambient_lux = update->u.d_uint32[0];
    }
    if (mask & LCD_MASK_AMBIENT_THRESHOLD) {
      s_data.ambient_threshold_lux = update->u.d_uint32[1];
    }

    if (mask & LCD_MASK_UID) {
      snprintf(s_runtime.uid, sizeof(s_runtime.uid), "%s", update->uid);
    }
    if (mask & LCD_MASK_MAC) {
      snprintf(s_runtime.mac, sizeof(s_runtime.mac), "%02X:%02X:%02X:%02X:%02X:%02X",
               update->u.d_uint8[0], update->u.d_uint8[1], update->u.d_uint8[2],
               update->u.d_uint8[3], update->u.d_uint8[4], update->u.d_uint8[5]);
    }
    if (mask & LCD_MASK_IP) {
      if (update->ip != 0) {
        esp_ip4_addr_t ip = { .addr = update->ip };
        snprintf(s_runtime.ip, sizeof(s_runtime.ip), IPSTR, IP2STR(&ip));
      }
    }

    s_data.update = mask;
    s_data.mask |= mask;

    xSemaphoreGive(s_state_mutex);
  }
}

/**
 * @brief Initializes LCD helper stack and starts UI.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t lcd_InitHelper(void) {
  esp_log_level_set(TAG, CONFIG_LCD_HELPER_LOG_LEVEL);
  ESP_LOGI(TAG, "++%s()", __func__);

  s_state_mutex = xSemaphoreCreateMutex();
  if (s_state_mutex == NULL) {
    return ESP_FAIL;
  }

  ESP_RETURN_ON_ERROR(lcd_InitHw(&s_lcd), TAG, "lcd_InitHw failed");
  ESP_RETURN_ON_ERROR(lcd_InitLvgl(&s_lcd), TAG, "lcd_InitLvgl failed");

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}


/**
 * @brief Stops LCD helper, releases LVGL resources and hardware.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t lcd_DoneHelper(void) {
  ESP_LOGI(TAG, "++%s()", __func__);

  if (s_ui_task != NULL) {
    vTaskDelete(s_ui_task);
    s_ui_task = NULL;
  }

  if (s_tick_timer != NULL) {
    esp_timer_stop(s_tick_timer);
    esp_timer_delete(s_tick_timer);
    s_tick_timer = NULL;
  }

  if (s_touch_indev != NULL) {
    lv_indev_delete(s_touch_indev);
    s_touch_indev = NULL;
  }

  if (s_display != NULL) {
    lv_display_delete(s_display);
    s_display = NULL;
  }

  lv_deinit();

  if (s_lcd.buffer1 != NULL) {
    free(s_lcd.buffer1);
    s_lcd.buffer1 = NULL;
  }
  if (s_lcd.buffer2 != NULL) {
    free(s_lcd.buffer2);
    s_lcd.buffer2 = NULL;
  }

  lcd_DoneHw();

  if (s_state_mutex != NULL) {
    vSemaphoreDelete(s_state_mutex);
    s_state_mutex = NULL;
  }

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}
