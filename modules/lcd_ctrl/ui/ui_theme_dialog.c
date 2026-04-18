/**
 * @file ui_theme_dialog.c
 * @brief Theme selector modal dialog (LVGL).
 *
 * Layout: title + one row per theme (gradient swatch | name | checkmark) + Close button.
 * Selecting a row applies the gradient immediately to the main screen background.
 */

#include "lvgl.h"

#include "ui_theme_dialog.h"
#include "ui_main_screen.h"

#define COLOR_OVERLAY_DIM   0x000000
#define COLOR_PANEL         0x121C28
#define COLOR_ACCENT        0x5DA7B1
#define COLOR_TEXT          0xFFFFFF
#define COLOR_MUTED         0x7A99B8
#define COLOR_BTN_BG        0x1E3048
#define COLOR_ROW_SEL       0x1A2C40
#define COLOR_ROW_DEFAULT   0x0D1520

#define SWATCH_W            36
#define SWATCH_H            20

static lv_obj_t* s_overlay = NULL;
static lv_obj_t* s_rows[UI_THEME_COUNT];
static lv_obj_t* s_checks[UI_THEME_COUNT];

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Refresh row highlight and checkmark visibility for the given active theme.
 */
static void theme_update_selection(ui_theme_id_t active) {
  for (int i = 0; i < (int)UI_THEME_COUNT; i++) {
    if (s_rows[i] == NULL) {
      continue;
    }
    bool sel = (i == (int)active);
    lv_obj_set_style_bg_color(s_rows[i],
      lv_color_hex(sel ? COLOR_ROW_SEL : COLOR_ROW_DEFAULT), 0);
    lv_obj_set_style_border_width(s_rows[i], sel ? 1 : 0, 0);
    lv_obj_set_style_border_color(s_rows[i], lv_color_hex(COLOR_ACCENT), 0);
    if (s_checks[i] != NULL) {
      if (sel) {
        lv_obj_remove_flag(s_checks[i], LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(s_checks[i], LV_OBJ_FLAG_HIDDEN);
      }
    }
  }
}

/**
 * @brief Called when a theme row is pressed; applies the theme and refreshes selection.
 */
static void theme_row_pressed_cb(lv_event_t* e) {
  ui_theme_id_t id = (ui_theme_id_t)(uintptr_t)lv_event_get_user_data(e);
  ui_main_screen_set_theme(id);
  theme_update_selection(id);
}

/**
 * @brief Called when the Close button is pressed; deletes the overlay.
 */
static void theme_dialog_close_cb(lv_event_t* e) {
  lv_obj_t* overlay = (lv_obj_t*)lv_event_get_user_data(e);
  if (overlay == NULL) {
    return;
  }
  lv_obj_delete(overlay);
  if (s_overlay == overlay) {
    s_overlay = NULL;
    for (int i = 0; i < (int)UI_THEME_COUNT; i++) {
      s_rows[i]   = NULL;
      s_checks[i] = NULL;
    }
  }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void ui_theme_dialog_show(void) {
  if (s_overlay != NULL) {
    lv_obj_delete(s_overlay);
    s_overlay = NULL;
  }
  for (int i = 0; i < (int)UI_THEME_COUNT; i++) {
    s_rows[i]   = NULL;
    s_checks[i] = NULL;
  }

  /* Full-screen dimming overlay on the LVGL top layer. */
  lv_obj_t* overlay = lv_obj_create(lv_layer_top());
  s_overlay = overlay;
  lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(overlay, lv_color_hex(COLOR_OVERLAY_DIM), 0);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
  lv_obj_set_style_border_width(overlay, 0, 0);
  lv_obj_set_style_pad_all(overlay, 0, 0);
  lv_obj_set_style_radius(overlay, 0, 0);
  lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

  /* Centered panel. */
  lv_obj_t* panel = lv_obj_create(overlay);
  lv_obj_set_width(panel, LV_PCT(88));
  lv_obj_set_height(panel, LV_SIZE_CONTENT);
  lv_obj_center(panel);
  lv_obj_set_style_bg_color(panel, lv_color_hex(COLOR_PANEL), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 10, 0);
  lv_obj_set_style_pad_all(panel, 10, 0);
  lv_obj_set_style_pad_row(panel, 6, 0);
  lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  /* Title. */
  lv_obj_t* title = lv_label_create(panel);
  lv_label_set_text(title, "Theme");
  lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);

  /* Theme rows. */
  ui_theme_id_t cur = ui_main_screen_get_theme();
  for (int i = 0; i < (int)UI_THEME_COUNT; i++) {
    const char* name  = NULL;
    uint32_t    top_c = 0;
    uint32_t    bot_c = 0;
    ui_main_screen_theme_info((ui_theme_id_t)i, &name, &top_c, &bot_c);

    /* Row button. */
    lv_obj_t* row = lv_obj_create(panel);
    s_rows[i] = row;
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_set_style_pad_all(row, 5, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(row, theme_row_pressed_cb, LV_EVENT_CLICKED,
                        (void*)(uintptr_t)(ui_theme_id_t)i);

    /* Gradient swatch: top color → bottom color, vertical. */
    lv_obj_t* swatch = lv_obj_create(row);
    lv_obj_set_size(swatch, SWATCH_W, SWATCH_H);
    lv_obj_set_style_radius(swatch, 4, 0);
    lv_obj_set_style_bg_color(swatch, lv_color_hex(top_c), 0);
    lv_obj_set_style_bg_grad_color(swatch, lv_color_hex(bot_c), 0);
    lv_obj_set_style_bg_grad_dir(swatch, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(swatch, lv_color_hex(COLOR_MUTED), 0);
    lv_obj_set_style_border_width(swatch, 1, 0);
    lv_obj_set_style_pad_all(swatch, 0, 0);
    lv_obj_remove_flag(swatch, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    /* Theme name label. */
    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, (name != NULL) ? name : "?");
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_flex_grow(lbl, 1);

    /* Checkmark — visible only on the active theme. */
    lv_obj_t* chk = lv_label_create(row);
    s_checks[i] = chk;
    lv_label_set_text(chk, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(chk, lv_color_hex(COLOR_ACCENT), 0);
    if (i != (int)cur) {
      lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);
    }
  }

  /* Apply initial row highlight. */
  theme_update_selection(cur);

  /* Footer: Close button. */
  lv_obj_t* foot = lv_obj_create(panel);
  lv_obj_remove_style_all(foot);
  lv_obj_set_width(foot, LV_PCT(100));
  lv_obj_set_height(foot, LV_SIZE_CONTENT);
  lv_obj_set_layout(foot, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* close_btn = lv_button_create(foot);
  lv_obj_set_size(close_btn, LV_PCT(60), 28);
  lv_obj_set_style_bg_color(close_btn, lv_color_hex(COLOR_BTN_BG), 0);
  lv_obj_set_style_bg_opa(close_btn, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(close_btn, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_border_width(close_btn, 1, 0);
  lv_obj_set_style_radius(close_btn, 8, 0);
  lv_obj_set_style_pad_all(close_btn, 0, 0);
  lv_obj_add_event_cb(close_btn, theme_dialog_close_cb, LV_EVENT_CLICKED, overlay);

  lv_obj_t* close_lbl = lv_label_create(close_btn);
  lv_label_set_text(close_lbl, "Close");
  lv_obj_set_style_text_color(close_lbl, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_center(close_lbl);
}

void ui_theme_dialog_close_if_open(void) {
  if (s_overlay != NULL) {
    lv_obj_delete(s_overlay);
    s_overlay = NULL;
    for (int i = 0; i < (int)UI_THEME_COUNT; i++) {
      s_rows[i]   = NULL;
      s_checks[i] = NULL;
    }
  }
}
