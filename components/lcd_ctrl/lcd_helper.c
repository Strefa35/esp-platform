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
#include <stdio.h>
#include <stdbool.h>
 
#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_err.h"

#include "lcd_defs.h"
#include "lcd_helper.h"
#include "lcd_hw.h"

#include "lvgl.h"


static lcd_t lcd = {
  .h_res = 0,
  .v_res = 0,
  .buffer_size = 0,
  .buffer1 = NULL,
  .buffer2 = NULL
};


static const char* TAG = "ESP::LCD::HELPER";


static lv_obj_t * btn;
static lv_display_rotation_t rotation = LV_DISP_ROTATION_0;

static void btn_cb(lv_event_t * e)
{
    lv_display_t *disp = lv_event_get_user_data(e);
    rotation++;
    if (rotation > LV_DISP_ROTATION_270) {
        rotation = LV_DISP_ROTATION_0;
    }
    lv_disp_set_rotation(disp, rotation);
}
static void set_angle(void * obj, int32_t v)
{
    lv_arc_set_value(obj, v);
}

void example_lvgl_demo_ui(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);

    btn = lv_button_create(scr);
    lv_obj_t * lbl = lv_label_create(btn);
    lv_label_set_text_static(lbl, LV_SYMBOL_REFRESH" ROTATE");
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    /*Button event*/
    lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, disp);

    /*Create an Arc*/
    lv_obj_t * arc = lv_arc_create(scr);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_center(arc);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, set_angle);
    lv_anim_set_duration(&a, 1000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);    /*Just for the demo*/
    lv_anim_set_repeat_delay(&a, 500);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_start(&a);
}

static esp_err_t lcd_InitLvgl(lcd_t* lcd_ptr) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "Initialize LVGL library");
  lv_init();

  // create a lvgl display
  lv_display_t *display = lv_display_create(lcd_ptr->h_res, lcd_ptr->v_res);

  // initialize LVGL draw buffers
  lv_display_set_buffers(display, lcd_ptr->buffer1, lcd_ptr->buffer2, lcd_ptr->buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
  // associate the mipi panel handle to the display
  lv_display_set_user_data(display, NULL/*panel_handle*/);
  // set color depth
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

#if 0
  // set the callback which can copy the rendered image to an area of the display
  lv_display_set_flush_cb(display, example_lvgl_flush_cb);

  ESP_LOGI(TAG, "Install LVGL tick timer");
  // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
  const esp_timer_create_args_t lvgl_tick_timer_args = {
      .callback = &example_increase_lvgl_tick,
      .name = "lvgl_tick"
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

  ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
  const esp_lcd_panel_io_callbacks_t cbs = {
      .on_color_trans_done = example_notify_lvgl_flush_ready,
  };
  /* Register done callback */
  ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display));
#endif

  example_lvgl_demo_ui(display);

  //lv_display_delete(display);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}


esp_err_t lcd_InitHelper() {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_LCD_HELPER_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = lcd_InitHw(&lcd);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] lcd_InitHw() - result: %d", __func__, result);
    return result;
  }

  result = lcd_InitLvgl(&lcd);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] lcd_InitLvgl() - result: %d", __func__, result);
    return result;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

