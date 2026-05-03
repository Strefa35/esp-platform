/**
 * @file ui_screensaver.c
 * @author A.Czerwinski@pistacje.net
 * @brief Screensaver UI (see docs/todo/lcd_mockup_screensaver.png)
 * @version 0.1
 * @date 2026-03-29
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 *
 */
#include <stdio.h>

#include "lvgl.h"

#include "ui_screensaver.h"

/* ------------------------------------------------------------------ */
/*  Colours                                                           */
/* ------------------------------------------------------------------ */
#define COLOR_BG            0x101820  /* dark navy background          */
#define COLOR_TIME          0xFFFFFF  /* white clock digits            */
#define COLOR_DATE          0x7A99B8  /* muted blue-grey date text     */
#define COLOR_ACCENT        0x3FA7D6  /* light-blue accent             */
#define COLOR_CLOCK_FACE    0x0B1320  /* analog clock face background  */
#define COLOR_TICK_MINOR    0xAABBCC  /* muted tick color              */
#define COLOR_PANEL_BG      0x0E1826  /* weather panel background       */

/* ------------------------------------------------------------------ */
/*  Layout constants                                                  */
/* ------------------------------------------------------------------ */
#define CLOCK_SIZE          155       /* analog clock diameter         */
#define CLOCK_HOUR_LEN      43
#define CLOCK_MIN_LEN       58
#define CLOCK_SEC_LEN       65
#define WEATHER_ICON_TEXT   LV_SYMBOL_IMAGE

/* ------------------------------------------------------------------ */
/*  LVGL object handles (module-private)                             */
/* ------------------------------------------------------------------ */
static lv_obj_t* s_label_date = NULL;
static lv_obj_t* s_label_weather_temp = NULL;
static lv_obj_t* s_label_weather_summary = NULL;
static lv_obj_t* s_label_weather_location = NULL;

#if LV_USE_SCALE
static lv_obj_t* s_clock_scale = NULL;
static lv_obj_t* s_hand_hour = NULL;
static lv_obj_t* s_hand_min = NULL;
static lv_obj_t* s_hand_sec = NULL;
static lv_point_precise_t s_minute_hand_points[2];
static lv_point_precise_t s_second_hand_points[2];
#else
static lv_obj_t* s_label_time = NULL;
#endif

/**
 * @brief Create the screensaver layout: clock (analog or digital fallback), date, weather panel.
 *
 * @param display Active LVGL display whose active screen is replaced.
 */
void ui_screensaver_create(lv_display_t* display) {
  lv_obj_t* scr = lv_display_get_screen_active(display);
  lv_obj_clean(scr);

  lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(scr, 0, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY,
                             LV_FLEX_ALIGN_CENTER,
                             LV_FLEX_ALIGN_CENTER);

  lv_obj_t* left_panel = lv_obj_create(scr);
  lv_obj_remove_style_all(left_panel);
  lv_obj_set_size(left_panel, LV_PCT(56), LV_PCT(100));
  lv_obj_set_layout(left_panel, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_CENTER,
                                    LV_FLEX_ALIGN_CENTER,
                                    LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_right(left_panel, 8, 0);

  lv_obj_t* right_panel = lv_obj_create(scr);
  lv_obj_set_size(right_panel, LV_PCT(40), LV_PCT(84));
  lv_obj_set_style_bg_color(right_panel, lv_color_hex(COLOR_PANEL_BG), 0);
  lv_obj_set_style_bg_opa(right_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(right_panel, 1, 0);
  lv_obj_set_style_border_color(right_panel, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_radius(right_panel, 10, 0);
  lv_obj_set_style_pad_hor(right_panel, 10, 0);
  lv_obj_set_style_pad_ver(right_panel, 12, 0);
  lv_obj_set_layout(right_panel, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(right_panel, LV_FLEX_ALIGN_CENTER,
                                     LV_FLEX_ALIGN_CENTER,
                                     LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(right_panel, 6, 0);

#if LV_USE_SCALE
  s_clock_scale = lv_scale_create(left_panel);
  lv_obj_set_size(s_clock_scale, CLOCK_SIZE, CLOCK_SIZE);
  lv_scale_set_mode(s_clock_scale, LV_SCALE_MODE_ROUND_INNER);
  lv_scale_set_range(s_clock_scale, 0, 60);
  lv_scale_set_total_tick_count(s_clock_scale, 61);
  lv_scale_set_major_tick_every(s_clock_scale, 5);
  lv_scale_set_angle_range(s_clock_scale, 360);
  lv_scale_set_rotation(s_clock_scale, 270);
  lv_scale_set_label_show(s_clock_scale, true);

  static const char* hour_ticks[] = {"12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", NULL};
  lv_scale_set_text_src(s_clock_scale, hour_ticks);

  lv_obj_set_style_bg_opa(s_clock_scale, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(s_clock_scale, lv_color_hex(COLOR_CLOCK_FACE), 0);
  lv_obj_set_style_radius(s_clock_scale, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(s_clock_scale, true, 0);

  static lv_style_t clock_major_style;
  static lv_style_t clock_minor_style;
  static lv_style_t clock_main_style;
  lv_style_init(&clock_major_style);
  lv_style_set_text_font(&clock_major_style, LV_FONT_DEFAULT);
  lv_style_set_text_color(&clock_major_style, lv_color_hex(COLOR_DATE));
  lv_style_set_line_color(&clock_major_style, lv_color_hex(COLOR_ACCENT));
  lv_style_set_length(&clock_major_style, 10);
  lv_style_set_line_width(&clock_major_style, 2);
  lv_obj_add_style(s_clock_scale, &clock_major_style, LV_PART_INDICATOR);

  lv_style_init(&clock_minor_style);
  lv_style_set_line_color(&clock_minor_style, lv_color_hex(COLOR_TICK_MINOR));
  lv_style_set_length(&clock_minor_style, 6);
  lv_style_set_line_width(&clock_minor_style, 1);
  lv_obj_add_style(s_clock_scale, &clock_minor_style, LV_PART_ITEMS);

  lv_style_init(&clock_main_style);
  lv_style_set_arc_color(&clock_main_style, lv_color_hex(COLOR_ACCENT));
  lv_style_set_arc_width(&clock_main_style, 2);
  lv_obj_add_style(s_clock_scale, &clock_main_style, LV_PART_MAIN);

  s_hand_sec = lv_line_create(s_clock_scale);
  lv_line_set_points_mutable(s_hand_sec, s_second_hand_points, 2);
  lv_obj_set_style_line_width(s_hand_sec, 2, 0);
  lv_obj_set_style_line_rounded(s_hand_sec, true, 0);
  lv_obj_set_style_line_color(s_hand_sec, lv_color_hex(COLOR_ACCENT), 0);

  s_hand_min = lv_line_create(s_clock_scale);
  lv_line_set_points_mutable(s_hand_min, s_minute_hand_points, 2);
  lv_obj_set_style_line_width(s_hand_min, 3, 0);
  lv_obj_set_style_line_rounded(s_hand_min, true, 0);
  lv_obj_set_style_line_color(s_hand_min, lv_color_hex(COLOR_TIME), 0);

  s_hand_hour = lv_line_create(s_clock_scale);
  lv_obj_set_style_line_width(s_hand_hour, 5, 0);
  lv_obj_set_style_line_rounded(s_hand_hour, true, 0);
  lv_obj_set_style_line_color(s_hand_hour, lv_color_hex(COLOR_TIME), 0);

  lv_obj_t* center_dot = lv_obj_create(s_clock_scale);
  lv_obj_set_size(center_dot, 8, 8);
  lv_obj_set_style_radius(center_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(center_dot, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_bg_opa(center_dot, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(center_dot, 0, 0);
  lv_obj_center(center_dot);
#else
  s_label_time = lv_label_create(left_panel);
  lv_label_set_text(s_label_time, "--:--:--");
  lv_obj_set_style_text_color(s_label_time, lv_color_hex(COLOR_TIME), 0);
  lv_obj_set_style_text_align(s_label_time, LV_TEXT_ALIGN_CENTER, 0);
#endif

  s_label_date = lv_label_create(left_panel);
  lv_label_set_text(s_label_date, "");
  lv_obj_set_style_text_color(s_label_date, lv_color_hex(COLOR_DATE), 0);
  lv_obj_set_style_text_align(s_label_date, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t* weather_icon = lv_label_create(right_panel);
  lv_label_set_text(weather_icon, WEATHER_ICON_TEXT);
  lv_obj_set_style_text_color(weather_icon, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_text_font(weather_icon, LV_FONT_DEFAULT, 0);

  s_label_weather_temp = lv_label_create(right_panel);
  lv_label_set_text(s_label_weather_temp, "-- C");
  lv_obj_set_style_text_color(s_label_weather_temp, lv_color_hex(COLOR_TIME), 0);
  lv_obj_set_style_text_align(s_label_weather_temp, LV_TEXT_ALIGN_CENTER, 0);

  s_label_weather_summary = lv_label_create(right_panel);
  lv_label_set_text(s_label_weather_summary, "No weather data");
  lv_obj_set_width(s_label_weather_summary, LV_PCT(100));
  lv_label_set_long_mode(s_label_weather_summary, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(s_label_weather_summary, lv_color_hex(COLOR_DATE), 0);
  lv_obj_set_style_text_align(s_label_weather_summary, LV_TEXT_ALIGN_CENTER, 0);

  s_label_weather_location = lv_label_create(right_panel);
  lv_label_set_text(s_label_weather_location, "-");
  lv_obj_set_style_text_color(s_label_weather_location, lv_color_hex(COLOR_TICK_MINOR), 0);
  lv_obj_set_style_text_align(s_label_weather_location, LV_TEXT_ALIGN_CENTER, 0);
}

/**
 * @brief Update clock hands or digital time label, and the date label.
 *
 * @param time_str "HH:MM:SS" when LV_USE_SCALE; any string for the digital fallback. NULL skips time.
 * @param date_str Date line; NULL leaves date unchanged.
 */
void ui_screensaver_update_time(const char* time_str, const char* date_str) {
  if (time_str != NULL) {
#if LV_USE_SCALE
    if (s_clock_scale != NULL && s_hand_hour != NULL && s_hand_min != NULL && s_hand_sec != NULL) {
      int hour = 0;
      int minute = 0;
      int second = 0;
      if (sscanf(time_str, "%2d:%2d:%2d", &hour, &minute, &second) == 3) {
        hour %= 12;
        minute %= 60;
        second %= 60;

        lv_scale_set_line_needle_value(s_clock_scale, s_hand_sec, CLOCK_SEC_LEN, second);
        lv_scale_set_line_needle_value(s_clock_scale, s_hand_min, CLOCK_MIN_LEN, minute);
        lv_scale_set_line_needle_value(s_clock_scale, s_hand_hour, CLOCK_HOUR_LEN, hour * 5 + (minute / 12));
      }
    }
#else
    if (s_label_time != NULL) {
      lv_label_set_text(s_label_time, time_str);
    }
#endif
  }

  if (s_label_date != NULL && date_str != NULL) {
    lv_label_set_text(s_label_date, date_str);
  }
}

/**
 * @brief Update temperature, summary, and location labels on the weather panel.
 *
 * @param temperature_str e.g. "21 C"; NULL leaves previous text.
 * @param summary_str       Short description; NULL leaves previous text.
 * @param location_str      Place name; NULL leaves previous text.
 */
void ui_screensaver_update_weather(const char* temperature_str,
                                   const char* summary_str,
                                   const char* location_str) {
  if (s_label_weather_temp != NULL && temperature_str != NULL) {
    lv_label_set_text(s_label_weather_temp, temperature_str);
  }
  if (s_label_weather_summary != NULL && summary_str != NULL) {
    lv_label_set_text(s_label_weather_summary, summary_str);
  }
  if (s_label_weather_location != NULL && location_str != NULL) {
    lv_label_set_text(s_label_weather_location, location_str);
  }
}
