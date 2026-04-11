/**
 * @file ui_mqtt_info_dialog.c
 * @brief MQTT info modal (LVGL).
 *
 * Layout: compact label|value rows inside a scroll area so the Close button stays visible
 * on small panels (e.g. 240px height); scrollbar appears when content exceeds the viewport.
 */

#include <stdio.h>
#include <string.h>

#include "lvgl.h"

#include "lcd_helper.h"
#include "ui_mqtt_info_dialog.h"

#define COLOR_OVERLAY_DIM    0x000000
#define COLOR_PANEL          0x121C28
#define COLOR_ACCENT         0x5DA7B1
#define COLOR_TEXT           0xFFFFFF
#define COLOR_MUTED          0x7A99B8
#define COLOR_BTN_BG         0x1E3048

#define CLOSE_BTN_SIZE_PCT   150
#define MQTT_LINK_REFRESH_MS 400

static lv_obj_t* s_overlay = NULL;
static lv_obj_t* s_mqtt_link_lbl = NULL;
static lv_timer_t* s_mqtt_link_timer = NULL;

static void mqtt_dialog_stop_link_refresh(void) {
  if (s_mqtt_link_timer != NULL) {
    lv_timer_delete(s_mqtt_link_timer);
    s_mqtt_link_timer = NULL;
  }
  s_mqtt_link_lbl = NULL;
}

static void mqtt_link_label_set(lv_obj_t* lbl, bool up) {
  if (lbl == NULL) {
    return;
  }
  lv_label_set_text(lbl, up ? "Connected" : "Disconnected");
  lv_obj_set_style_text_color(lbl, lv_color_hex(up ? COLOR_ACCENT : COLOR_MUTED), 0);
}

static void mqtt_link_refresh_timer_cb(lv_timer_t* t) {
  (void)t;
  if (s_mqtt_link_lbl == NULL) {
    return;
  }
  mqtt_link_label_set(s_mqtt_link_lbl, lcd_GetMqttLinkConnected());
}

static lv_coord_t scale_dim_pct(lv_coord_t base, unsigned pct) {
  return (lv_coord_t)(((int32_t)base * (int32_t)pct + 50) / 100);
}

static void measure_text_extents(lv_obj_t* parent,
                                 const char* text,
                                 lv_obj_t* style_ref,
                                 lv_coord_t* out_w,
                                 lv_coord_t* out_h) {
  lv_obj_t* m = lv_label_create(parent);
  lv_label_set_text(m, text);
  if (style_ref != NULL) {
    lv_obj_set_style_text_font(m, lv_obj_get_style_text_font(style_ref, LV_PART_MAIN), LV_PART_MAIN);
  }
  lv_obj_add_flag(m, LV_OBJ_FLAG_HIDDEN);
  lv_obj_update_layout(m);
  if (out_w != NULL) {
    lv_coord_t w = lv_obj_get_width(m);
    *out_w = w > 0 ? w : 1;
  }
  if (out_h != NULL) {
    lv_coord_t h = lv_obj_get_height(m);
    *out_h = h > 0 ? h : 1;
  }
  lv_obj_delete(m);
}

static void mqtt_dialog_close_cb(lv_event_t* e) {
  lv_obj_t* overlay = (lv_obj_t*)lv_event_get_user_data(e);
  if (overlay == NULL) {
    return;
  }
  mqtt_dialog_stop_link_refresh();
  lv_obj_delete(overlay);
  if (s_overlay == overlay) {
    s_overlay = NULL;
  }
}

static void add_compact_row(lv_obj_t* col, const char* label, const char* value) {
  lv_obj_t* row = lv_obj_create(col);
  lv_obj_remove_style_all(row);
  lv_obj_set_width(row, LV_PCT(100));
  lv_obj_set_height(row, LV_SIZE_CONTENT);
  lv_obj_set_layout(row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_column(row, 6, 0);
  lv_obj_set_style_pad_top(row, 1, 0);
  lv_obj_set_style_pad_bottom(row, 1, 0);

  lv_obj_t* l = lv_label_create(row);
  lv_label_set_text(l, label);
  lv_obj_set_style_text_color(l, lv_color_hex(COLOR_MUTED), 0);
  lv_obj_set_width(l, LV_PCT(34));
  lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);

  lv_obj_t* v = lv_label_create(row);
  lv_label_set_text(v, (value != NULL && value[0] != '\0') ? value : "---");
  lv_obj_set_style_text_color(v, lv_color_hex(COLOR_TEXT), 0);
  lv_obj_set_flex_grow(v, 1);
  lv_label_set_long_mode(v, LV_LABEL_LONG_WRAP);
}

void ui_mqtt_info_dialog_show(const ui_mqtt_info_dialog_data_t* data) {
  if (data == NULL) {
    return;
  }

  if (s_overlay != NULL) {
    mqtt_dialog_stop_link_refresh();
    lv_obj_delete(s_overlay);
    s_overlay = NULL;
  }

  lv_obj_t* top = lv_layer_top();
  lv_obj_t* overlay = lv_obj_create(top);
  s_overlay = overlay;
  lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(overlay, lv_color_hex(COLOR_OVERLAY_DIM), 0);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
  lv_obj_set_style_border_width(overlay, 0, 0);
  lv_obj_set_style_pad_all(overlay, 0, 0);
  lv_obj_set_style_radius(overlay, 0, 0);
  lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

  lv_display_t* disp = lv_obj_get_display(overlay);
  lv_coord_t vres = disp != NULL ? lv_display_get_vertical_resolution(disp) : 240;
  lv_coord_t scroll_h = (vres * 46) / 100;
  if (scroll_h < 72) {
    scroll_h = 72;
  }

  lv_obj_t* panel = lv_obj_create(overlay);
  lv_obj_set_width(panel, LV_PCT(88));
  lv_obj_set_height(panel, LV_SIZE_CONTENT);
  lv_obj_set_style_max_height(panel, (vres * 90) / 100, 0);
  lv_obj_center(panel);
  lv_obj_set_style_bg_color(panel, lv_color_hex(COLOR_PANEL), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 10, 0);
  lv_obj_set_style_pad_all(panel, 8, 0);
  lv_obj_set_style_pad_row(panel, 5, 0);
  lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* head = lv_obj_create(panel);
  lv_obj_remove_style_all(head);
  lv_obj_set_width(head, LV_PCT(100));
  lv_obj_set_height(head, LV_SIZE_CONTENT);
  lv_obj_set_layout(head, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(head, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* title = lv_label_create(head);
  lv_label_set_text(title, "MQTT");
  lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);

  lv_obj_t* link_lbl = lv_label_create(head);
  s_mqtt_link_lbl = link_lbl;
  mqtt_link_label_set(link_lbl, data->link_up);
  s_mqtt_link_timer = lv_timer_create(mqtt_link_refresh_timer_cb, MQTT_LINK_REFRESH_MS, NULL);

  lv_obj_t* scroll = lv_obj_create(panel);
  lv_obj_set_width(scroll, LV_PCT(100));
  lv_obj_set_height(scroll, scroll_h);
  lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(scroll, 0, 0);
  lv_obj_set_style_pad_all(scroll, 0, 0);
  lv_obj_set_style_pad_right(scroll, 4, 0);
  lv_obj_add_flag(scroll, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_AUTO);

  lv_obj_t* inner = lv_obj_create(scroll);
  lv_obj_remove_style_all(inner);
  lv_obj_set_width(inner, LV_PCT(100));
  lv_obj_set_height(inner, LV_SIZE_CONTENT);
  lv_obj_set_layout(inner, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(inner, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(inner, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(inner, 4, 0);

  add_compact_row(inner, "Broker", data->broker);
  add_compact_row(inner, "UID", data->uid);
  add_compact_row(inner, "Req", data->req_topic);

  lv_coord_t close_tw = 1;
  lv_coord_t close_th = 1;
  measure_text_extents(panel, "Close", link_lbl, &close_tw, &close_th);
  lv_coord_t close_bw = scale_dim_pct(close_tw, CLOSE_BTN_SIZE_PCT);
  lv_coord_t close_bh = scale_dim_pct(close_th, CLOSE_BTN_SIZE_PCT);

  lv_obj_t* foot = lv_obj_create(panel);
  lv_obj_remove_style_all(foot);
  lv_obj_set_width(foot, LV_PCT(100));
  lv_obj_set_height(foot, LV_SIZE_CONTENT);
  lv_obj_set_layout(foot, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* close_btn = lv_button_create(foot);
  lv_obj_set_width(close_btn, close_bw);
  lv_obj_set_height(close_btn, close_bh);
  lv_obj_set_style_pad_hor(close_btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_ver(close_btn, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(COLOR_BTN_BG), 0);
  lv_obj_set_style_bg_opa(close_btn, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(close_btn, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_border_width(close_btn, 1, 0);
  lv_obj_set_style_radius(close_btn, 8, 0);
  lv_obj_add_event_cb(close_btn, mqtt_dialog_close_cb, LV_EVENT_CLICKED, overlay);

  lv_obj_t* close_lbl = lv_label_create(close_btn);
  lv_label_set_text(close_lbl, "Close");
  lv_obj_set_style_text_color(close_lbl, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_center(close_lbl);
}

void ui_mqtt_info_dialog_close_if_open(void) {
  mqtt_dialog_stop_link_refresh();
  if (s_overlay != NULL) {
    lv_obj_delete(s_overlay);
    s_overlay = NULL;
  }
}
