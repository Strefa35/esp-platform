/**
 * @file ui_main_screen.c
 * @author A.Czerwinski@pistacje.net
 * @brief Main screen UI (see docs/todo/lcd_mockup_main.png)
 * @version 0.1
 * @date 2026-03-29
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lvgl.h"

#include "ui_main_screen.h"
#include "ui_material_icons.h"

/* ------------------------------------------------------------------ */
/*  Colours (aligned with LCD mockup)                                 */
/* ------------------------------------------------------------------ */
#define COLOR_BG            0x0A111A
#define COLOR_STATUS_BAR    0x0D1520
#define COLOR_ACCENT        0x5DA7B1
#define COLOR_ICON_DIM      0x4A6270
#define COLOR_TIME          0xFFFFFF
#define COLOR_DATE          0x7A99B8
#define COLOR_BTN_BG        0x1E3048
#define COLOR_PANEL_BG      0x121C28
#define COLOR_PILL_BG       0x0D1520
/** Horizontal lux pill: deep night blue (left/moon) → warm sunlight amber (right/sun). */
#define COLOR_LUX_GRAD_L    0x0A1628
#define COLOR_LUX_GRAD_R    0xFFA726
#define COLOR_LUX_MARKER    0xFFFFFF
#define COLOR_LUX_THR_LINE  0xE53935
/** Unlit zone of the lux pill (right of the current lux marker). */
#define COLOR_LUX_UNLIT     0x121C28

/* ------------------------------------------------------------------ */
/*  Layout                                                            */
/* ------------------------------------------------------------------ */
#define STATUS_BAR_H        40
/** Fixed-height strip under `body` for ambient lux pill (LUX_PILL_H + 2×pad_ver). */
#define BOTTOM_BAR_H        48

#define LEFT_COL_PCT        60
#define RIGHT_COL_PCT       40

/** Inset from body top for `clock_area` (matches `docs/diagrams/ui_main_screen_layout.svg`). */
#define LEFT_COL_PAD_TOP    6
/** Horizontal inset for `clock_area` inside the left column (diagram: x=6, width 180 in 192px column). */
#define LEFT_COL_PAD_HOR    6
/** Height of `clock_area` (time row); sized for Montserrat 38 HM + 14 sec on one baseline. */
#define CLOCK_AREA_H        56
/** Vertical gap between the time row (`clock_area`) and the date label below it. */
#define CLOCK_DATE_GAP      4
#define LUX_PILL_H          36
#define LUX_SPAN_DEFAULT    4000u
#define LUX_MARKER_W        3

/* ------------------------------------------------------------------ */
/*  Widget handles                                                    */
/* ------------------------------------------------------------------ */
static lv_obj_t* s_icon_eth = NULL;
static lv_obj_t* s_icon_wifi = NULL;
static lv_obj_t* s_icon_mqtt = NULL;
static lv_obj_t* s_icon_bt = NULL;
static lv_obj_t* s_lbl_hm = NULL;
static lv_obj_t* s_lbl_sec = NULL;
static lv_obj_t* s_label_date = NULL;
/** Lux pill: gradient track; gray cover hides unlit zone (right of lux marker);
 *  red thr_line = threshold; white val_line = current lux; overlay = moon/text/sun. */
static lv_obj_t* s_lux_caption = NULL;
static lv_obj_t* s_lux_track = NULL;
static lv_obj_t* s_lux_gray_cover = NULL;
static lv_obj_t* s_lux_thr_line = NULL;
static lv_obj_t* s_lux_val_line = NULL;
static lv_obj_t* s_lux_overlay = NULL;
static lv_obj_t* s_lux_val_lbl = NULL;
static lv_obj_t* s_lux_thr_lbl = NULL;
static lv_obj_t* s_heater_state = NULL;
static lv_obj_t* s_pump_state = NULL;

static uint32_t s_lux_snap = 0;
static uint32_t s_thr_snap = 0;
static uint32_t s_lux_span = LUX_SPAN_DEFAULT;

/* ------------------------------------------------------------------ */
/*  Theme table                                                        */
/* ------------------------------------------------------------------ */
typedef struct {
  const char* name;
  uint32_t    color_top;
  uint32_t    color_bot;
} ui_theme_def_t;

static const ui_theme_def_t s_themes[UI_THEME_COUNT] = {
  [UI_THEME_DEEP_OCEAN]  = { "Deep Ocean",  0x1E4060, 0x0A1520 },
  [UI_THEME_NORDIC_HOME] = { "Nordic Home", 0x1C3048, 0x0A111A },
  [UI_THEME_WARM_SLATE]  = { "Warm Slate",  0x282038, 0x100C1E },
  [UI_THEME_SUNRISE]     = { "Sunrise",     0x1E3828, 0x0A1510 },
};

static ui_theme_id_t s_active_theme = UI_THEME_DEEP_OCEAN;
static lv_obj_t*     s_scr = NULL;

/**
 * @brief Tint a status-bar material icon: accent when active, dim when inactive.
 *
 * @param icon   Material icon label; NULL is a no-op.
 * @param active true uses accent color, false uses dimmed color.
 */
static void set_status_icon_color(lv_obj_t* icon, bool active) {
  if (icon == NULL) {
    return;
  }
  lv_obj_set_style_text_color(icon,
    active ? lv_color_hex(COLOR_ACCENT) : lv_color_hex(COLOR_ICON_DIM), 0);
}

/**
 * @brief Apply the bundled Material Icons font to a label, if non-NULL.
 *
 * @param obj LVGL object (typically a label).
 */
static void apply_material_icon_font(lv_obj_t* obj) {
  if (obj != NULL) {
    lv_obj_set_style_text_font(obj, &lv_font_material_icons_22, 0);
  }
}

/**
 * @brief Create a fixed-size status-bar icon slot with a centered Material icon.
 *
 * Using a dedicated button-sized slot keeps the touch hitbox aligned with the
 * visible icon, unlike a raw clickable label whose text bounds can feel offset.
 *
 * @param parent   Parent row in the status bar.
 * @param symbol   Material icon glyph string.
 * @param on_press Optional click callback; NULL creates a non-clickable slot.
 * @return lv_obj_t* Inner label used later for tint updates.
 */
static lv_obj_t* make_status_icon_slot(lv_obj_t* parent, const char* symbol, lv_event_cb_t on_press, void* user_data) {
  static const lv_style_prop_t transition_props[] = {
    LV_STYLE_BG_OPA,
    LV_STYLE_BORDER_WIDTH,
    LV_STYLE_OUTLINE_WIDTH,
    0
  };
  static lv_style_transition_dsc_t transition_dsc;
  static bool transition_ready = false;

  if (!transition_ready) {
    lv_style_transition_dsc_init(&transition_dsc, transition_props, lv_anim_path_ease_out, 120, 0, NULL);
    transition_ready = true;
  }

  lv_obj_t* slot = lv_button_create(parent);
  lv_obj_remove_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(slot, 40, 32);
  lv_obj_set_style_radius(slot, 6, 0);
  lv_obj_set_style_pad_all(slot, 0, 0);
  lv_obj_set_style_shadow_width(slot, 0, 0);
  lv_obj_set_style_transition(slot, &transition_dsc, LV_PART_MAIN);

  /* Default state: visually quiet, but still a real pressable control. */
  lv_obj_set_style_bg_opa(slot, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(slot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_width(slot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  /* Pressed state: subtle feedback matching the rest of the UI. */
  lv_obj_set_style_bg_color(slot, lv_color_hex(COLOR_BTN_BG), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(slot, LV_OPA_60, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_border_color(slot, lv_color_hex(COLOR_ACCENT), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_border_width(slot, 1, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_outline_color(slot, lv_color_hex(COLOR_ACCENT), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_outline_opa(slot, LV_OPA_50, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_outline_width(slot, 1, LV_PART_MAIN | LV_STATE_PRESSED);

  if (on_press != NULL) {
    /* Resistive touch can jitter during release, so react on press and keep the
     * target locked even if the finger slides slightly. */
    lv_obj_add_flag(slot, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(slot, on_press, LV_EVENT_PRESSED, user_data);
  } else {
    lv_obj_remove_flag(slot, LV_OBJ_FLAG_CLICKABLE);
  }

  lv_obj_t* icon = lv_label_create(slot);
  lv_label_set_text(icon, symbol);
  apply_material_icon_font(icon);
  lv_obj_center(icon);
  return icon;
}

/**
 * @brief Apply the active theme as a vertical gradient to the screen background.
 *
 * @param scr LVGL screen object; NULL is a no-op.
 */
static void apply_bg_gradient(lv_obj_t* scr) {
  if (scr == NULL) {
    return;
  }
  const ui_theme_def_t* t = &s_themes[s_active_theme];
  lv_obj_set_style_bg_color(scr, lv_color_hex(t->color_top), 0);
  lv_obj_set_style_bg_grad_color(scr, lv_color_hex(t->color_bot), 0);
  lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}

/**
 * @brief Set fonts on the digital clock: HM uses Montserrat 38 when enabled in LVGL config, with fallbacks;
 *        seconds and date use 14pt when enabled.
 */
static void apply_clock_fonts(void) {
#if LV_FONT_MONTSERRAT_38
  if (s_lbl_hm != NULL) {
    lv_obj_set_style_text_font(s_lbl_hm, &lv_font_montserrat_38, 0);
  }
#elif LV_FONT_MONTSERRAT_32
  if (s_lbl_hm != NULL) {
    lv_obj_set_style_text_font(s_lbl_hm, &lv_font_montserrat_32, 0);
  }
#elif LV_FONT_MONTSERRAT_28
  if (s_lbl_hm != NULL) {
    lv_obj_set_style_text_font(s_lbl_hm, &lv_font_montserrat_28, 0);
  }
#elif LV_FONT_MONTSERRAT_46
  if (s_lbl_hm != NULL) {
    lv_obj_set_style_text_font(s_lbl_hm, &lv_font_montserrat_46, 0);
  }
#endif
#if LV_FONT_MONTSERRAT_14
  if (s_lbl_sec != NULL) {
    lv_obj_set_style_text_font(s_lbl_sec, &lv_font_montserrat_14, 0);
  }
  if (s_label_date != NULL) {
    lv_obj_set_style_text_font(s_label_date, &lv_font_montserrat_14, 0);
  }
#endif
}

/**
 * @brief Position lux/threshold markers and stretch the text overlay to the pill content box.
 */
static void layout_lux_markers(void) {
  if (s_lux_track == NULL || s_lux_thr_line == NULL || s_lux_val_line == NULL || s_lux_gray_cover == NULL) {
    return;
  }
  lv_obj_update_layout(s_lux_track);
  lv_coord_t w = lv_obj_get_content_width(s_lux_track);
  lv_coord_t h = lv_obj_get_content_height(s_lux_track);
  if (w < 8 || h < 4) {
    return;
  }
  uint32_t span = s_lux_span;
  if (span < 1u) {
    span = 1u;
  }
  if (s_lux_overlay != NULL) {
    lv_obj_set_size(s_lux_overlay, w, h);
    lv_obj_set_pos(s_lux_overlay, 0, 0);
  }

  lv_coord_t line_w = (lv_coord_t)LUX_MARKER_W;
  if (line_w > w / 2) {
    line_w = w / 4;
    if (line_w < 2) {
      line_w = 2;
    }
  }

  uint32_t thr_clamped = (s_thr_snap > span) ? span : s_thr_snap;
  lv_coord_t xt = (lv_coord_t)((uint64_t)thr_clamped * (uint64_t)(w - line_w) / (uint64_t)span);
  if (xt < 0) {
    xt = 0;
  }
  if (xt > w - line_w) {
    xt = w - line_w;
  }
  lv_obj_set_pos(s_lux_thr_line, xt, 0);
  lv_obj_set_size(s_lux_thr_line, line_w, h);

  uint32_t lux_clamped = (s_lux_snap > span) ? span : s_lux_snap;
  lv_coord_t xv = (lv_coord_t)((uint64_t)lux_clamped * (uint64_t)(w - line_w) / (uint64_t)span);
  if (xv < 0) {
    xv = 0;
  }
  if (xv > w - line_w) {
    xv = w - line_w;
  }
  lv_obj_set_pos(s_lux_val_line, xv, 0);
  lv_obj_set_size(s_lux_val_line, line_w, h);

  /* Gray cover: hides gradient from the right edge of val_line to the pill end. */
  lv_coord_t cover_x = xv + line_w;
  lv_coord_t cover_w = w - cover_x;
  if (cover_w < 0) {
    cover_w = 0;
  }
  lv_obj_set_pos(s_lux_gray_cover, cover_x, 0);
  lv_obj_set_size(s_lux_gray_cover, cover_w, h);
}

/**
 * @brief Apply Montserrat to ambient lux labels when enabled in LVGL config.
 */
static void apply_lux_bar_fonts(void) {
#if LV_FONT_MONTSERRAT_14
  if (s_lux_caption != NULL) {
    lv_obj_set_style_text_font(s_lux_caption, &lv_font_montserrat_14, 0);
  }
  if (s_lux_val_lbl != NULL) {
    lv_obj_set_style_text_font(s_lux_val_lbl, &lv_font_montserrat_14, 0);
  }
  if (s_lux_thr_lbl != NULL) {
    lv_obj_set_style_text_font(s_lux_thr_lbl, &lv_font_montserrat_14, 0);
  }
#endif
}

/**
 * @brief Create a small pill-shaped button with a centered symbol label.
 *
 * @param parent   Flex row/column parent.
 * @param sym      LVGL symbol or text for the label.
 * @param on_press Optional callback fired on `LV_EVENT_PRESSED`; NULL disables clicking.
 * @return         The button object (child label centered inside).
 */
static lv_obj_t* make_pill_button(lv_obj_t* parent, const char* sym, lv_event_cb_t on_press, void* user_data) {
  lv_obj_t* b = lv_button_create(parent);
  lv_obj_set_size(b, 26, 22);
  lv_obj_set_style_bg_color(b, lv_color_hex(COLOR_PILL_BG), 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(b, 1, 0);
  lv_obj_set_style_border_color(b, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_radius(b, 11, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(COLOR_BTN_BG), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(b, LV_OPA_80, LV_PART_MAIN | LV_STATE_PRESSED);
  if (on_press != NULL) {
    lv_obj_add_flag(b, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(b, on_press, LV_EVENT_PRESSED, user_data);
  } else {
    lv_obj_remove_flag(b, LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_t* l = lv_label_create(b);
  lv_label_set_text(l, sym);
  lv_obj_set_style_text_color(l, lv_color_hex(COLOR_TIME), 0);
  lv_obj_center(l);
  return b;
}

/**
 * @brief Add a relay control block: title row, status line, and +/- row with center state.
 *
 * @param parent                 Right-panel column parent.
 * @param title                  Section title text (plain label).
 * @param header_symbol          Material icon glyph string for the header.
 * @param pump_play_pause_center If true, center shows play/pause icon; else "OFF"/"ON" text.
 * @param state_lbl_out          Optional; receives the center state label for later updates.
 * @param on_ui_event            Shared event callback (NULL disables both pills).
 * @param off_event              Event ID passed as user_data for the "−" pill.
 * @param on_event               Event ID passed as user_data for the "+" pill.
 */
static void add_relay_section(lv_obj_t* parent,
                              const char* title,
                              const char* header_symbol,
                              bool pump_play_pause_center,
                              lv_obj_t** state_lbl_out,
                              lv_event_cb_t on_ui_event,
                              ui_main_event_id_t off_event,
                              ui_main_event_id_t on_event) {
  lv_obj_t* box = lv_obj_create(parent);
  lv_obj_remove_style_all(box);
  lv_obj_set_width(box, LV_PCT(100));
  lv_obj_set_height(box, LV_SIZE_CONTENT);
  lv_obj_set_layout(box, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(box, 4, 0);

  lv_obj_t* head = lv_obj_create(box);
  lv_obj_remove_style_all(head);
  lv_obj_set_size(head, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(head, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(head, 6, 0);

  /* Section header: Material icon glyph (accent color) beside the title text. */
  lv_obj_t* hi = lv_label_create(head);
  lv_label_set_text(hi, header_symbol);
  lv_obj_set_style_text_color(hi, lv_color_hex(COLOR_ACCENT), 0);
  apply_material_icon_font(hi);

  /* Section title text, e.g. "water heater" or "Circulation pump". */
  lv_obj_t* ht = lv_label_create(head);
  lv_label_set_text(ht, title);
  lv_obj_set_style_text_color(ht, lv_color_hex(COLOR_TIME), 0);

  /* Static "Status" caption above the control row. */
  lv_obj_t* st = lv_label_create(box);
  lv_label_set_text(st, "Status");
  lv_obj_set_style_text_color(st, lv_color_hex(COLOR_DATE), 0);

  /* Static "Control:" label above the −/state/+ pill row. */
  lv_obj_t* ctrl_lbl = lv_label_create(box);
  lv_label_set_text(ctrl_lbl, "Control:");
  lv_obj_set_style_text_color(ctrl_lbl, lv_color_hex(COLOR_DATE), 0);

  lv_obj_t* row = lv_obj_create(box);
  lv_obj_remove_style_all(row);
  lv_obj_set_width(row, LV_PCT(100));
  lv_obj_set_height(row, LV_SIZE_CONTENT);
  lv_obj_set_layout(row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 4, 0);

  /* "−" pill button — turns the relay OFF when pressed. */
  (void) make_pill_button(row, LV_SYMBOL_MINUS, on_ui_event, (void*)(uintptr_t)off_event);

  /* Center state pill: shows "ON"/"OFF" text (heater) or play/pause icon (pump). */
  lv_obj_t* mid = lv_obj_create(row);
  lv_obj_remove_style_all(mid);
  lv_obj_set_flex_grow(mid, 1);
  lv_obj_set_height(mid, 24);
  lv_obj_set_style_bg_color(mid, lv_color_hex(COLOR_PILL_BG), 0);
  lv_obj_set_style_bg_opa(mid, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(mid, 1, 0);
  lv_obj_set_style_border_color(mid, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_radius(mid, 12, 0);
  lv_obj_set_layout(mid, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(mid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  /* State label inside the center pill — updated at runtime via ui_main_screen_update_relays(). */
  lv_obj_t* state = lv_label_create(mid);
  if (pump_play_pause_center) {
    lv_label_set_text(state, MAT_ICON_PAUSE);
    apply_material_icon_font(state);
  } else {
    lv_label_set_text(state, "OFF");
  }
  lv_obj_set_style_text_color(state, lv_color_hex(COLOR_TIME), 0);
  lv_obj_set_style_text_align(state, LV_TEXT_ALIGN_CENTER, 0);
  if (state_lbl_out != NULL) {
    *state_lbl_out = state;
  }

  /* "+" pill button — turns the relay ON when pressed. */
  (void) make_pill_button(row, LV_SYMBOL_PLUS, on_ui_event, (void*)(uintptr_t)on_event);
}

/**
 * @brief Build and attach the status bar (connectivity icons + settings button) to the screen.
 *
 * @param scr          Active LVGL screen object.
 * @param on_ui_event  Shared event callback; each widget passes its UI_MAIN_EVENT_* as user_data.
 */
static void create_top_bar(lv_obj_t* scr, lv_event_cb_t on_ui_event) {
  /* --- Status bar: top strip, full width, accent bottom border --- */
  lv_obj_t* status_bar = lv_obj_create(scr);
  lv_obj_set_size(status_bar, LV_PCT(100), STATUS_BAR_H);
  lv_obj_set_flex_grow(status_bar, 0);
  lv_obj_set_style_bg_color(status_bar, lv_color_hex(COLOR_STATUS_BAR), 0);
  lv_obj_set_style_bg_opa(status_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(status_bar, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(status_bar, 1, 0);
  lv_obj_set_style_border_color(status_bar, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_radius(status_bar, 0, 0);
  lv_obj_set_style_pad_hor(status_bar, 8, 0);
  lv_obj_set_layout(status_bar, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                                     LV_FLEX_ALIGN_CENTER,
                                     LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

  /* Row of four status labels (stored in s_icon_* for later color updates). */
  lv_obj_t* icons = lv_obj_create(status_bar);
  lv_obj_remove_style_all(icons);
  lv_obj_set_layout(icons, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(icons, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(icons, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(icons, 16, 0);
  lv_obj_set_size(icons, LV_SIZE_CONTENT, LV_PCT(100));
  lv_obj_remove_flag(icons, LV_OBJ_FLAG_SCROLLABLE);

  /* Ethernet LAN icon — accent when link is up, dim when disconnected. */
  s_icon_eth = make_status_icon_slot(icons, MAT_ICON_LAN, on_ui_event, (void*)(uintptr_t)UI_MAIN_EVENT_ETH);
  set_status_icon_color(s_icon_eth, false);

  /* Wi-Fi icon — accent when associated to an AP, dim otherwise. */
  s_icon_wifi = make_status_icon_slot(icons, MAT_ICON_WIFI, on_ui_event, (void*)(uintptr_t)UI_MAIN_EVENT_WIFI);
  set_status_icon_color(s_icon_wifi, false);

  /* MQTT cloud icon — accent when broker is connected; tap opens broker status dialog. */
  s_icon_mqtt = make_status_icon_slot(icons, MAT_ICON_CLOUD, on_ui_event, (void*)(uintptr_t)UI_MAIN_EVENT_MQTT);
  set_status_icon_color(s_icon_mqtt, false);

  /* Bluetooth icon — reserved for future BT integration; always rendered dim. */
  s_icon_bt = make_status_icon_slot(icons, MAT_ICON_BLUETOOTH, NULL, NULL);
  set_status_icon_color(s_icon_bt, false);

  /* Settings button — rounded dark tile on the right end of the status bar. */
  lv_obj_t* cfg_btn = lv_button_create(status_bar);
  lv_obj_remove_flag(cfg_btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(cfg_btn, 32, 26);
  lv_obj_set_style_bg_color(cfg_btn, lv_color_hex(COLOR_BTN_BG), 0);
  lv_obj_set_style_bg_opa(cfg_btn, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(cfg_btn, 1, 0);
  lv_obj_set_style_border_color(cfg_btn, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_radius(cfg_btn, 6, 0);
  lv_obj_set_style_pad_all(cfg_btn, 0, 0);
  if (on_ui_event != NULL) {
    lv_obj_add_event_cb(cfg_btn, on_ui_event, LV_EVENT_CLICKED, (void*)(uintptr_t)UI_MAIN_EVENT_CONFIG);
  }
  /* Gear glyph centered inside the settings button. */
  lv_obj_t* cfg_icon = lv_label_create(cfg_btn);
  lv_label_set_text(cfg_icon, MAT_ICON_SETTINGS);
  apply_material_icon_font(cfg_icon);
  lv_obj_set_style_text_color(cfg_icon, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_center(cfg_icon);
}

/**
 * @brief Build and attach the left clock column (digital clock + date label) to the body row.
 *
 * Writes to module-level widget handles: s_lbl_hm, s_lbl_sec, s_label_date.
 *
 * @param parent Body row container (flex row).
 */
static void create_clock_col(lv_obj_t* parent) {
  /* Left column: clock + date only (ambient lux is in bottom_bar). */
  lv_obj_t* left = lv_obj_create(parent);
  lv_obj_remove_style_all(left);
  lv_obj_set_size(left, LV_PCT(LEFT_COL_PCT), LV_PCT(100));
  lv_obj_set_layout(left, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(left, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_top(left, LEFT_COL_PAD_TOP, 0);
  lv_obj_set_style_pad_hor(left, LEFT_COL_PAD_HOR, 0);
  lv_obj_set_style_pad_row(left, CLOCK_DATE_GAP, 0);

  /* Digital clock: `clock_area` is full width × CLOCK_AREA_H; time row only inside that band. */
  lv_obj_t* clock_area = lv_obj_create(left);
  lv_obj_remove_style_all(clock_area);
  lv_obj_set_width(clock_area, LV_PCT(100));
  lv_obj_set_height(clock_area, CLOCK_AREA_H);
  //lv_obj_set_style_border_width(clock_area, 1, 0);
  //lv_obj_set_style_border_color(clock_area, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_layout(clock_area, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(clock_area, LV_FLEX_FLOW_COLUMN);
  /* One band filling the clock height; time row is centered inside it. */
  lv_obj_set_flex_align(clock_area, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_add_flag(clock_area, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_remove_flag(clock_area, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* time_row = lv_obj_create(clock_area);
  lv_obj_remove_style_all(time_row);
  lv_obj_set_width(time_row, LV_PCT(100));
  lv_obj_set_height(time_row, LV_PCT(100));
  lv_obj_set_flex_grow(time_row, 1);
  lv_obj_set_layout(time_row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
  /* Group centered horizontally; bottom-align cross-axis so HM + seconds share one visual baseline. */
  lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(time_row, 2, 0);
  lv_obj_add_flag(time_row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_remove_flag(time_row, LV_OBJ_FLAG_SCROLLABLE);

  /* HH:MM. and SS on one row (dot separates minutes from seconds). */
  s_lbl_hm = lv_label_create(time_row);
  lv_label_set_text(s_lbl_hm, "--:--.");
  lv_obj_set_style_text_color(s_lbl_hm, lv_color_hex(COLOR_TIME), 0);

  s_lbl_sec = lv_label_create(time_row);
  lv_label_set_text(s_lbl_sec, "--");
  lv_obj_set_style_text_color(s_lbl_sec, lv_color_hex(COLOR_TIME), 0);

  /* Date sits below the clock band. */
  s_label_date = lv_label_create(left);
  lv_label_set_text(s_label_date, "");
  lv_obj_set_style_text_color(s_label_date, lv_color_hex(COLOR_DATE), 0);
  lv_obj_set_style_text_align(s_label_date, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(s_label_date, LV_PCT(100));

  apply_clock_fonts();
}

/**
 * @brief Build and attach the right relay panel (water heater + circulation pump) to the body row.
 *
 * Writes to module-level widget handles: s_heater_state, s_pump_state.
 *
 * @param parent      Body row container (flex row).
 * @param on_ui_event Shared event callback; pills pass UI_MAIN_EVENT_* as user_data.
 */
static void create_relay_panel(lv_obj_t* parent, lv_event_cb_t on_ui_event) {
  /* Right column: narrow side card with relay widgets (heater text state, pump play/pause icon). */
  lv_obj_t* right = lv_obj_create(parent);
  lv_obj_remove_style_all(right);
  lv_obj_set_size(right, LV_PCT(RIGHT_COL_PCT), LV_PCT(100));
  /* Diagram: panel at x=198 vs right column x=192 → 6px horizontal inset. */
  lv_obj_set_style_pad_left(right, 6, 0);
  lv_obj_set_style_pad_right(right, 6, 0);
  lv_obj_set_style_pad_ver(right, 6, 0);
  lv_obj_remove_flag(right, LV_OBJ_FLAG_SCROLLABLE);

  /* Bordered flex column; children spaced evenly (two add_relay_section stacks). */
  lv_obj_t* panel = lv_obj_create(right);
  lv_obj_set_size(panel, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(panel, lv_color_hex(COLOR_PANEL_BG), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_radius(panel, 12, 0);
  lv_obj_set_style_pad_hor(panel, 8, 0);
  lv_obj_set_style_pad_ver(panel, 10, 0);
  lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  /* Heater: center label shows ON/OFF text. Pump: center shows Material play/pause icons. */
  add_relay_section(panel, "water heater", MAT_ICON_WAVES, false, &s_heater_state,
                    on_ui_event, UI_MAIN_EVENT_HEATER_OFF, UI_MAIN_EVENT_HEATER_ON);
  add_relay_section(panel, "Circulation pump", MAT_ICON_SYNC, true, &s_pump_state,
                    on_ui_event, UI_MAIN_EVENT_PUMP_OFF, UI_MAIN_EVENT_PUMP_ON);
}

/**
 * @brief Build and attach the body area (clock column + relay panel) to the screen.
 *
 * @param scr          Active LVGL screen object.
 * @param on_ui_event  Shared event callback; relay pills pass their UI_MAIN_EVENT_* as user_data.
 */
static void create_body_bar(lv_obj_t* scr, lv_event_cb_t on_ui_event) {
  /* --- Body: horizontal split (main content below status bar) --- */
  lv_obj_t* body_bar = lv_obj_create(scr);
  lv_obj_remove_style_all(body_bar);
  lv_obj_set_width(body_bar, LV_PCT(100));
  lv_obj_set_height(body_bar, 0);
  lv_obj_set_flex_grow(body_bar, 1);
  lv_obj_set_layout(body_bar, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(body_bar, LV_FLEX_FLOW_ROW);
  /* LVGL flex has no STRETCH; children use LV_PCT(100) height to fill this row. */
  lv_obj_set_flex_align(body_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(body_bar, LV_OBJ_FLAG_SCROLLABLE);

  create_clock_col(body_bar);
  //create_relay_panel(body_bar, on_ui_event);
}

/**
 * @brief Build and attach the bottom bar (ambient lux pill) to the screen.
 *
 * @param scr Active LVGL screen object.
 */
static void create_bottom_bar(lv_obj_t* scr) {
  /* Bottom strip: ambient lux, full width (matches schematic bottom_bar). */
  lv_obj_t* bottom_bar = lv_obj_create(scr);
  lv_obj_remove_style_all(bottom_bar);
  lv_obj_set_size(bottom_bar, LV_PCT(100), BOTTOM_BAR_H);
  lv_obj_set_flex_grow(bottom_bar, 0);
  lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(COLOR_PANEL_BG), 0);
  lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(bottom_bar, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_border_width(bottom_bar, 1, 0);
  lv_obj_set_style_border_color(bottom_bar, lv_color_hex(COLOR_ACCENT), 0);
  lv_obj_set_style_pad_hor(bottom_bar, 6, 0);
  lv_obj_set_style_pad_ver(bottom_bar, 4, 0);
  lv_obj_set_layout(bottom_bar, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(bottom_bar, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(bottom_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_remove_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

  /* Pill wrapper: full width, sized to content (just the gradient pill). */
  lv_obj_t* lux_col = lv_obj_create(bottom_bar);
  lv_obj_remove_style_all(lux_col);
  lv_obj_set_width(lux_col, LV_PCT(100));
  lv_obj_set_height(lux_col, LV_SIZE_CONTENT);
  lv_obj_set_layout(lux_col, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(lux_col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(lux_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_hor(lux_col, 4, 0);
  lv_obj_remove_flag(lux_col, LV_OBJ_FLAG_SCROLLABLE);

  /* Gradient pill track: deep night blue (left/moon) → warm sunlight amber (right/sun). */
  s_lux_track = lv_obj_create(lux_col);
  lv_obj_remove_flag(s_lux_track, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_width(s_lux_track, LV_PCT(100));
  lv_obj_set_height(s_lux_track, LUX_PILL_H);
  lv_obj_set_style_bg_opa(s_lux_track, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(s_lux_track, lv_color_hex(COLOR_LUX_GRAD_L), 0);
  lv_obj_set_style_bg_grad_color(s_lux_track, lv_color_hex(COLOR_LUX_GRAD_R), 0);
  lv_obj_set_style_bg_grad_dir(s_lux_track, LV_GRAD_DIR_HOR, 0);
  lv_obj_set_style_radius(s_lux_track, LUX_PILL_H / 2, 0);
  lv_obj_set_style_clip_corner(s_lux_track, true, 0);
  lv_obj_set_style_pad_all(s_lux_track, 0, 0);
  lv_obj_set_style_border_width(s_lux_track, 0, 0);

  /* Unlit zone cover: solid COLOR_LUX_UNLIT rectangle from val_line right edge to pill end.
   * Drawn first so threshold and value markers render on top of it. */
  s_lux_gray_cover = lv_obj_create(s_lux_track);
  lv_obj_remove_style_all(s_lux_gray_cover);
  lv_obj_set_style_bg_opa(s_lux_gray_cover, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(s_lux_gray_cover, lv_color_hex(COLOR_LUX_UNLIT), 0);
  lv_obj_set_style_border_width(s_lux_gray_cover, 0, 0);
  lv_obj_set_style_radius(s_lux_gray_cover, 0, 0);
  lv_obj_add_flag(s_lux_gray_cover, LV_OBJ_FLAG_FLOATING);

  /* Red vertical line — marks the lux threshold position on the pill. */
  s_lux_thr_line = lv_obj_create(s_lux_track);
  lv_obj_remove_style_all(s_lux_thr_line);
  lv_obj_set_style_bg_opa(s_lux_thr_line, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(s_lux_thr_line, lv_color_hex(COLOR_LUX_THR_LINE), 0);
  lv_obj_set_style_border_width(s_lux_thr_line, 0, 0);
  lv_obj_set_style_radius(s_lux_thr_line, LUX_MARKER_W, 0);
  lv_obj_add_flag(s_lux_thr_line, LV_OBJ_FLAG_FLOATING);

  /* White vertical line — marks the current measured lux position on the pill. */
  s_lux_val_line = lv_obj_create(s_lux_track);
  lv_obj_remove_style_all(s_lux_val_line);
  lv_obj_set_style_bg_opa(s_lux_val_line, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(s_lux_val_line, lv_color_hex(COLOR_LUX_MARKER), 0);
  lv_obj_set_style_border_width(s_lux_val_line, 0, 0);
  lv_obj_set_style_radius(s_lux_val_line, LUX_MARKER_W, 0);
  lv_obj_add_flag(s_lux_val_line, LV_OBJ_FLAG_FLOATING);

  /* Transparent overlay floating above the pill: moon (left) | value+threshold text | sun (right). */
  s_lux_overlay = lv_obj_create(s_lux_track);
  lv_obj_remove_style_all(s_lux_overlay);
  lv_obj_add_flag(s_lux_overlay, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_style_bg_opa(s_lux_overlay, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(s_lux_overlay, 0, 0);
  lv_obj_set_layout(s_lux_overlay, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(s_lux_overlay, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(s_lux_overlay, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_hor(s_lux_overlay, 8, 0);
  lv_obj_remove_flag(s_lux_overlay, LV_OBJ_FLAG_SCROLLABLE);

  /* Moon (brightness_3) glyph — left edge of the pill, dark/night side. */
  lv_obj_t* moon = lv_label_create(s_lux_overlay);
  lv_label_set_text(moon, MAT_ICON_MOON);
  apply_material_icon_font(moon);
  lv_obj_set_style_text_color(moon, lv_color_hex(COLOR_TIME), 0);

  /* Two-line text column: current value (top) and threshold (bottom); centered between icons. */
  lv_obj_t* text_col = lv_obj_create(s_lux_overlay);
  lv_obj_remove_style_all(text_col);
  lv_obj_set_height(text_col, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(text_col, 1);
  lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(text_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(text_col, 0, 0);
  lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, 0);
  lv_obj_remove_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);

  /* Current illuminance reading, e.g. "1234 lx" — updated by ui_main_screen_update_ambient_lux(). */
  s_lux_val_lbl = lv_label_create(text_col);
  lv_label_set_text(s_lux_val_lbl, "0 lx");
  lv_obj_set_style_text_color(s_lux_val_lbl, lv_color_hex(COLOR_TIME), 0);
  lv_obj_set_style_text_align(s_lux_val_lbl, LV_TEXT_ALIGN_CENTER, 0);

  /* Threshold line, e.g. "threshold 800 lx" — updated by ui_main_screen_update_ambient_lux(). */
  s_lux_thr_lbl = lv_label_create(text_col);
  lv_label_set_text(s_lux_thr_lbl, "threshold 0 lx");
  lv_obj_set_style_text_color(s_lux_thr_lbl, lv_color_hex(COLOR_DATE), 0);
  lv_obj_set_style_text_align(s_lux_thr_lbl, LV_TEXT_ALIGN_CENTER, 0);

  /* Sun (WB_SUNNY) glyph — right edge of the pill, bright/day side. */
  lv_obj_t* sun = lv_label_create(s_lux_overlay);
  lv_label_set_text(sun, MAT_ICON_WB_SUNNY);
  apply_material_icon_font(sun);
  lv_obj_set_style_text_color(sun, lv_color_hex(COLOR_TIME), 0);

  apply_lux_bar_fonts();

  s_lux_snap = 0;
  s_thr_snap = 0;
  s_lux_span = LUX_SPAN_DEFAULT;
  layout_lux_markers();
}

/**
 * @brief Build the main screen on the display's active screen (status bar, clock, lux, relays).
 *
 * @param display      Active LVGL display.
 * @param on_ui_event  Single callback for all interactive widgets; NULL disables all interactions.
 *                     Use `(ui_main_event_id_t)(uintptr_t)lv_event_get_user_data(e)` to identify the source.
 */
void ui_main_screen_create(lv_display_t* display, lv_event_cb_t on_ui_event) {
  lv_obj_t* scr = lv_display_get_screen_active(display);
  lv_obj_clean(scr);

  /*
   * Screen tree (top-to-bottom, left-to-right):
   *
   *   scr (column flex)
   *   ├── status_bar  fixed height; row: [connectivity icons] ... [settings button]
   *   │       icons: Ethernet, Wi-Fi, MQTT, Bluetooth (Material icons; tint via ui_main_screen_update_*)
   *   │       cfg_btn: opens config when on_config_pressed is set
   *   ├── body      row flex, grows vertically; splits main content (clock + relays only)
   *   │       ├── left   ~LEFT_COL_PCT% width, column: clock_area (time row, fixed height), then date label
   *   │       └── right  ~RIGHT_COL_PCT% width; padded wrapper + panel (rounded card)
   *   │               panel: two relay sections (water heater, circulation pump) via add_relay_section()
   *   └── bottom_bar  fixed height BOTTOM_BAR_H; full width; lux_col → caption + lux pill (gradient + marker + labels)
   *       (see docs/diagrams/ui_main_screen_layout.svg)
   */

  s_scr = scr;
  apply_bg_gradient(scr);
  lv_obj_set_style_pad_all(scr, 0, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  /* Default lv_obj is scrollable; keep the main layout fixed (status bar must not move on drag). */
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  create_top_bar(scr, on_ui_event);
  create_body_bar(scr, on_ui_event);
  create_bottom_bar(scr);
}

/**
 * @brief Update digital clock labels from a time string and optional date string.
 *
 * @param time_str Expects leading "HH:MM:SS" or "HH:MM"; NULL skips time update.
 * @param date_str  Full date line; NULL skips date update.
 */
void ui_main_screen_update_time(const char* time_str, const char* date_str) {
  if (time_str != NULL && s_lbl_hm != NULL && s_lbl_sec != NULL) {
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (sscanf(time_str, "%2d:%2d:%2d", &hour, &minute, &second) >= 2) {
      char buf[8];
      (void) snprintf(buf, sizeof(buf), "%02d:%02d.", hour, minute);
      lv_label_set_text(s_lbl_hm, buf);
      (void) snprintf(buf, sizeof(buf), "%02d", second);
      lv_label_set_text(s_lbl_sec, buf);
    }
  }
  if (s_label_date != NULL && date_str != NULL) {
    lv_label_set_text(s_label_date, date_str);
  }
}

/**
 * @brief Refresh lux pill scale, red threshold marker, white value marker, and labels.
 *
 * @param lux               Current illuminance (lux).
 * @param threshold_lux     Threshold for daylight logic (shown as sub-label text).
 * @param display_span_max  Bar full scale; values under 100 fall back to default span.
 */
void ui_main_screen_update_ambient_lux(uint32_t lux, uint32_t threshold_lux, uint32_t display_span_max) {
  s_lux_snap = lux;
  s_thr_snap = threshold_lux;
  s_lux_span = display_span_max;
  if (s_lux_span < 100u) {
    s_lux_span = LUX_SPAN_DEFAULT;
  }
  uint32_t hi = lux;
  if (threshold_lux > hi) {
    hi = threshold_lux;
  }
  if (hi > s_lux_span / 2u) {
    s_lux_span = hi + hi / 4u;
    if (s_lux_span < 500u) {
      s_lux_span = 500u;
    }
  }

  if (s_lux_val_lbl != NULL) {
    char buf[24];
    (void) snprintf(buf, sizeof(buf), "%lu lx", (unsigned long)lux);
    lv_label_set_text(s_lux_val_lbl, buf);
  }
  if (s_lux_thr_lbl != NULL) {
    char buf[32];
    (void) snprintf(buf, sizeof(buf), "threshold %lu lx", (unsigned long)threshold_lux);
    lv_label_set_text(s_lux_thr_lbl, buf);
  }

  layout_lux_markers();
}

/**
 * @brief Update heater text state and pump play/pause icon in the relay panel.
 *
 * @param water_heater_on      true shows "ON", false shows "OFF" for the heater row.
 * @param circulation_pump_on  true shows play icon, false shows pause icon for the pump row.
 */
void ui_main_screen_update_relays(bool water_heater_on, bool circulation_pump_on) {
  if (s_heater_state != NULL) {
    lv_label_set_text(s_heater_state, water_heater_on ? "ON" : "OFF");
  }
  if (s_pump_state != NULL) {
    if (circulation_pump_on) {
      lv_label_set_text(s_pump_state, MAT_ICON_PLAY);
    } else {
      lv_label_set_text(s_pump_state, MAT_ICON_PAUSE);
    }
  }
}

/**
 * @brief Set Ethernet status icon tint.
 *
 * @param connected true = accent, false = dimmed.
 */
void ui_main_screen_update_eth(bool connected) {
  set_status_icon_color(s_icon_eth, connected);
}

/**
 * @brief Set Wi-Fi status icon tint.
 *
 * @param connected true = accent, false = dimmed.
 */
void ui_main_screen_update_wifi(bool connected) {
  set_status_icon_color(s_icon_wifi, connected);
}

/**
 * @brief Set MQTT status icon tint.
 *
 * @param connected true = accent, false = dimmed.
 */
void ui_main_screen_update_mqtt(bool connected) {
  set_status_icon_color(s_icon_mqtt, connected);
}

/**
 * @brief Set Bluetooth status icon tint.
 *
 * @param connected true = accent, false = dimmed.
 */
void ui_main_screen_update_bluetooth(bool connected) {
  set_status_icon_color(s_icon_bt, connected);
}

void ui_main_screen_set_theme(ui_theme_id_t theme) {
  if (theme >= UI_THEME_COUNT) {
    return;
  }
  s_active_theme = theme;
  apply_bg_gradient(s_scr);
}

ui_theme_id_t ui_main_screen_get_theme(void) {
  return s_active_theme;
}

void ui_main_screen_theme_info(ui_theme_id_t id,
                               const char**  name_out,
                               uint32_t*     top_out,
                               uint32_t*     bot_out) {
  if (id >= UI_THEME_COUNT) {
    return;
  }
  if (name_out != NULL) { *name_out = s_themes[id].name; }
  if (top_out  != NULL) { *top_out  = s_themes[id].color_top; }
  if (bot_out  != NULL) { *bot_out  = s_themes[id].color_bot; }
}
