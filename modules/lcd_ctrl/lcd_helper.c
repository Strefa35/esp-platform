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
#include "esp_netif.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "lwip/ip4_addr.h"

#include "ili9341v.h"
#include "lcd_defs.h"
#include "lcd_helper.h"
#include "lcd_hw.h"
#include "lvgl.h"
#include "ns2009.h"


#define LCD_UI_TASK_NAME             "lcd-ui"
#define LCD_UI_TASK_STACK_SIZE       6144
#define LCD_UI_TASK_PRIORITY         9

#define LCD_LVGL_TICK_MS             2
#define LCD_LVGL_HANDLER_MS          10

#define LCD_UI_NAV_HEIGHT            38
#define LCD_UI_PAGES_HEIGHT          184
#define LCD_UI_TEXT_COLOR            0x000000


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
  LCD_PAGE_TIME = 0,
  LCD_PAGE_CONFIG,
  LCD_PAGE_DEVICE,
  LCD_PAGE_MAX,
} lcd_page_e;

static lcd_page_e s_active_page = LCD_PAGE_TIME;

static lv_obj_t* s_menu_buttons[LCD_PAGE_MAX] = {NULL};
static lv_obj_t* s_page_containers[LCD_PAGE_MAX] = {NULL};

static lv_obj_t* s_label_time_value = NULL;
static lv_obj_t* s_label_cfg = NULL;
static lv_obj_t* s_label_dev = NULL;
static lv_obj_t* s_label_ctrl = NULL;


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
  if (ns2009_GetTouch(&touch) == ESP_OK) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = touch.x;
    data->point.y = touch.y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/**
 * @brief Updates cached IP address from Ethernet netif when available.
 */
static void lcd_try_update_ip_from_eth(void) {
  esp_netif_t* eth_netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
  if (eth_netif == NULL) {
    return;
  }

  esp_netif_ip_info_t ip_info = {0};
  if (esp_netif_get_ip_info(eth_netif, &ip_info) != ESP_OK || ip_info.ip.addr == 0) {
    return;
  }

  ip4_addr_t ip_addr = {
    .addr = ip_info.ip.addr,
  };

  if (s_state_mutex != NULL && xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    snprintf(s_runtime.ip, sizeof(s_runtime.ip), IPSTR, IP2STR(&ip_addr));
    xSemaphoreGive(s_state_mutex);
  }
}

/**
 * @brief Builds comma-separated list of enabled controllers.
 *
 * @param out Output buffer.
 * @param out_len Output buffer size.
 */
static void lcd_build_controller_list(char* out, size_t out_len) {
  size_t used = 0;
  out[0] = '\0';

#define LCD_APPEND_CTRL(_cfg, _name) \
  do { \
    if (_cfg) { \
      int n = snprintf(out + used, (used < out_len) ? (out_len - used) : 0, \
                       "%s%s", (used == 0) ? "" : ", ", _name); \
      if (n > 0) { \
        used += (size_t) n; \
        if (used >= out_len) { \
          out[out_len - 1] = '\0'; \
          return; \
        } \
      } \
    } \
  } while (0)

#ifdef CONFIG_ETH_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "eth");
#endif
#ifdef CONFIG_GPIO_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "gpio");
#endif
#ifdef CONFIG_POWER_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "power");
#endif
#ifdef CONFIG_RELAY_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "relay");
#endif
#ifdef CONFIG_LCD_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "lcd");
#endif
#ifdef CONFIG_CFG_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "cfg");
#endif
#ifdef CONFIG_SYS_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "sys");
#endif
#ifdef CONFIG_CLI_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "cli");
#endif
#ifdef CONFIG_SENSOR_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "sensor");
#endif
#ifdef CONFIG_TEMPLATE_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "template");
#endif
#ifdef CONFIG_MQTT_CTRL_ENABLE
  LCD_APPEND_CTRL(1, "mqtt");
#endif

#undef LCD_APPEND_CTRL

  if (out[0] == '\0') {
    snprintf(out, out_len, "none");
  }
}

/**
 * @brief Periodically refreshes UI labels with time and runtime data.
 *
 * @param timer LVGL timer instance (unused).
 */
static void lcd_ui_update_timer_cb(lv_timer_t* timer) {
  (void) timer;

  char time_text[48] = {0};
  char cfg_text[256] = {0};
  char dev_text[256] = {0};
  char ctrl_text[256] = {0};

  time_t now = time(NULL);
  struct tm local_tm = {0};
  localtime_r(&now, &local_tm);
  strftime(time_text, sizeof(time_text), "%H:%M:%S", &local_tm);

  lcd_try_update_ip_from_eth();

  const char* tz = getenv("TZ");
  if (tz == NULL || tz[0] == '\0') {
#ifdef CONFIG_SYS_CTRL_TIMEZONE_CUSTOM
    tz = CONFIG_SYS_CTRL_TIMEZONE_CUSTOM_STRING;
#else
    tz = CONFIG_SYS_CTRL_TIMEZONE_STRING;
#endif
  }

#ifdef CONFIG_SYS_CTRL_NTP_SERVER_DEFAULT
  const char* ntp_server = CONFIG_SYS_CTRL_NTP_SERVER_DEFAULT;
#else
  const char* ntp_server = "n/a";
#endif

  char uid[sizeof(s_runtime.uid)] = {0};
  char mac[sizeof(s_runtime.mac)] = {0};
  char ip[sizeof(s_runtime.ip)] = {0};
  if (s_state_mutex != NULL && xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    snprintf(uid, sizeof(uid), "%s", s_runtime.uid);
    snprintf(mac, sizeof(mac), "%s", s_runtime.mac);
    snprintf(ip, sizeof(ip), "%s", s_runtime.ip);
    xSemaphoreGive(s_state_mutex);
  }

  snprintf(cfg_text, sizeof(cfg_text), "Time zone: %s\nNTP server: %s", tz ? tz : "UTC0", ntp_server);
  snprintf(dev_text, sizeof(dev_text), "UID: %s\nMAC: %s\nIP: %s", uid, mac, ip);
  lcd_build_controller_list(ctrl_text, sizeof(ctrl_text));

  if (s_label_time_value != NULL) {
    lv_label_set_text_fmt(s_label_time_value, "%s", time_text);
  }
  if (s_label_cfg != NULL) {
    lv_label_set_text(s_label_cfg, cfg_text);
  }
  if (s_label_dev != NULL) {
    lv_label_set_text(s_label_dev, dev_text);
  }
  if (s_label_ctrl != NULL) {
    lv_label_set_text_fmt(s_label_ctrl, "Controllers: %s", ctrl_text);
  }
}

/**
 * @brief Switches visible page and highlights selected navigation button.
 *
 * @param page Page index to activate.
 */
static void lcd_set_active_page(lcd_page_e page) {
  if (page >= LCD_PAGE_MAX) {
    return;
  }

  s_active_page = page;
  for (int idx = 0; idx < (int) LCD_PAGE_MAX; ++idx) {
    if (s_page_containers[idx] != NULL) {
      if (idx == (int) page) {
        lv_obj_clear_flag(s_page_containers[idx], LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(s_page_containers[idx], LV_OBJ_FLAG_HIDDEN);
      }
    }

    if (s_menu_buttons[idx] != NULL) {
      lv_color_t bg = (idx == (int) page) ? lv_color_hex(0x3FA7D6) : lv_color_hex(0x2A3B4D);
      lv_obj_set_style_bg_color(s_menu_buttons[idx], bg, 0);
    }
  }
}

/**
 * @brief Handles navigation button click event.
 *
 * @param e LVGL event with selected page index in user data.
 */
static void lcd_menu_btn_cb(lv_event_t* e) {
  intptr_t idx = (intptr_t) lv_event_get_user_data(e);
  lcd_set_active_page((lcd_page_e) idx);
}

/**
 * @brief Creates complete LCD user interface layout and widgets.
 *
 * @param display LVGL display used to create screen objects.
 */
static void lcd_build_ui(lv_display_t* display) {
  lv_obj_t* scr = lv_display_get_screen_active(display);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(scr, 6, 0);

  lv_obj_t* panel = lv_obj_create(scr);
  lv_obj_set_size(panel, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x1A2533), 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x3FA7D6), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_pad_all(panel, 6, 0);
  lv_obj_set_style_radius(panel, 6, 0);

  lv_obj_t* pages = lv_obj_create(panel);
  lv_obj_set_size(pages, LV_PCT(100), LCD_UI_PAGES_HEIGHT);
  lv_obj_align(pages, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_opa(pages, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(pages, 0, 0);
  lv_obj_set_style_pad_all(pages, 0, 0);

  lv_obj_t* nav = lv_obj_create(panel);
  lv_obj_set_size(nav, LV_PCT(100), LCD_UI_NAV_HEIGHT);
  lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(nav, lv_color_hex(0x13202C), 0);
  lv_obj_set_style_border_width(nav, 0, 0);
  lv_obj_set_style_pad_all(nav, 4, 0);
  lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  static const char* page_names[LCD_PAGE_MAX] = {
    "Time",
    "Config",
    "Device",
  };

  for (int idx = 0; idx < (int) LCD_PAGE_MAX; ++idx) {
    s_menu_buttons[idx] = lv_button_create(nav);
    lv_obj_set_size(s_menu_buttons[idx], 96, LV_PCT(100));
    lv_obj_set_style_bg_color(s_menu_buttons[idx], lv_color_hex(0x2A3B4D), 0);
    lv_obj_set_style_border_width(s_menu_buttons[idx], 0, 0);
    lv_obj_add_event_cb(s_menu_buttons[idx], lcd_menu_btn_cb, LV_EVENT_CLICKED, (void*) (intptr_t) idx);

    lv_obj_t* lbl = lv_label_create(s_menu_buttons[idx]);
    lv_label_set_text(lbl, page_names[idx]);
    lv_obj_center(lbl);
  }

  for (int idx = 0; idx < (int) LCD_PAGE_MAX; ++idx) {
    s_page_containers[idx] = lv_obj_create(pages);
    lv_obj_set_size(s_page_containers[idx], LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(s_page_containers[idx], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_page_containers[idx], 0, 0);
    lv_obj_set_style_pad_all(s_page_containers[idx], 0, 0);
    lv_obj_set_layout(s_page_containers[idx], LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_page_containers[idx], LV_FLEX_FLOW_COLUMN);
  }

  lv_obj_t* time_title = lv_label_create(s_page_containers[LCD_PAGE_TIME]);
  lv_obj_set_style_text_color(time_title, lv_color_hex(LCD_UI_TEXT_COLOR), 0);
  lv_label_set_text(time_title, "Current time");

  s_label_time_value = lv_label_create(s_page_containers[LCD_PAGE_TIME]);
  lv_obj_set_style_text_color(s_label_time_value, lv_color_hex(LCD_UI_TEXT_COLOR), 0);
  lv_obj_set_style_text_font(s_label_time_value, LV_FONT_DEFAULT, 0);
  lv_label_set_text(s_label_time_value, "--:--:--");

  s_label_cfg = lv_label_create(s_page_containers[LCD_PAGE_CONFIG]);
  lv_obj_set_style_text_color(s_label_cfg, lv_color_hex(LCD_UI_TEXT_COLOR), 0);
  lv_label_set_long_mode(s_label_cfg, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s_label_cfg, LV_PCT(100));

  s_label_dev = lv_label_create(s_page_containers[LCD_PAGE_DEVICE]);
  lv_obj_set_style_text_color(s_label_dev, lv_color_hex(LCD_UI_TEXT_COLOR), 0);
  lv_label_set_long_mode(s_label_dev, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s_label_dev, LV_PCT(100));

  s_label_ctrl = lv_label_create(s_page_containers[LCD_PAGE_DEVICE]);
  lv_obj_set_style_text_color(s_label_ctrl, lv_color_hex(LCD_UI_TEXT_COLOR), 0);
  lv_label_set_long_mode(s_label_ctrl, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s_label_ctrl, LV_PCT(100));

  lcd_set_active_page(LCD_PAGE_TIME);

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

  lcd_build_ui(s_display);

  BaseType_t ok = xTaskCreate(lcd_lvgl_task, LCD_UI_TASK_NAME, LCD_UI_TASK_STACK_SIZE, NULL, LCD_UI_TASK_PRIORITY, &s_ui_task);
  if (ok != pdPASS) {
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}

/**
 * @brief Updates device UID shown on LCD.
 *
 * @param uid Zero-terminated UID string.
 */
void lcd_UpdateUid(const char* uid) {
  if (uid == NULL || s_state_mutex == NULL) {
    return;
  }

  if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    snprintf(s_runtime.uid, sizeof(s_runtime.uid), "%s", uid);
    xSemaphoreGive(s_state_mutex);
  }
}

/**
 * @brief Updates device MAC address shown on LCD.
 *
 * @param mac Pointer to 6-byte MAC address.
 */
void lcd_UpdateMac(const uint8_t mac[6]) {
  if (mac == NULL || s_state_mutex == NULL) {
    return;
  }

  if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    snprintf(s_runtime.mac, sizeof(s_runtime.mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    xSemaphoreGive(s_state_mutex);
  }
}

/**
 * @brief Updates device IP address shown on LCD.
 *
 * @param ip_addr IPv4 address in network byte order.
 */
void lcd_UpdateIp(uint32_t ip_addr) {
  if (s_state_mutex == NULL || ip_addr == 0) {
    return;
  }

  ip4_addr_t ip = {
    .addr = ip_addr,
  };

  if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    snprintf(s_runtime.ip, sizeof(s_runtime.ip), IPSTR, IP2STR(&ip));
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
