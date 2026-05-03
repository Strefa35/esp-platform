/**
 * @file ui_main_screen.h
 * @author A.Czerwinski@pistacje.net
 * @brief Main LCD screen (layout: docs/todo/lcd_mockup_main.png)
 * @version 0.1
 * @date 2026-03-29
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 *
 */

#ifndef __UI_MAIN_SCREEN_H__
#define __UI_MAIN_SCREEN_H__

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

/**
 * @brief Identifies which UI element triggered an event dispatched via @p on_ui_event.
 *
 * Read with: `ui_main_event_id_t id = (ui_main_event_id_t)(uintptr_t)lv_event_get_user_data(e);`
 */
typedef enum {
  UI_MAIN_EVENT_CONFIG      = 0, /**< Settings (gear) button pressed. */
  UI_MAIN_EVENT_ETH         = 1, /**< Ethernet status icon pressed. */
  UI_MAIN_EVENT_WIFI        = 2, /**< Wi-Fi status icon pressed. */
  UI_MAIN_EVENT_MQTT        = 3, /**< MQTT status icon pressed. */
  UI_MAIN_EVENT_HEATER_OFF  = 4, /**< Water heater "−" pill pressed. */
  UI_MAIN_EVENT_HEATER_ON   = 5, /**< Water heater "+" pill pressed. */
  UI_MAIN_EVENT_PUMP_OFF    = 6, /**< Circulation pump "−" pill pressed. */
  UI_MAIN_EVENT_PUMP_ON     = 7, /**< Circulation pump "+" pill pressed. */
} ui_main_event_id_t;

/**
 * @brief Identifies the active background gradient theme.
 *
 * Each entry maps to a vertical 2-stop gradient applied to the main screen background.
 * Use @p ui_main_screen_theme_info() to query display names and colors.
 */
typedef enum {
  UI_THEME_DEEP_OCEAN   = 0, /**< Cool teal-to-navy gradient.          */
  UI_THEME_NORDIC_HOME  = 1, /**< Neutral steel-blue gradient.         */
  UI_THEME_WARM_SLATE   = 2, /**< Dark purple-slate gradient.          */
  UI_THEME_SUNRISE      = 3, /**< Dark forest-green gradient.          */
  UI_THEME_COUNT        = 4,
} ui_theme_id_t;

/**
 * @brief Switch the background gradient theme.
 *
 * Applies immediately when the main screen is active; remembered for the next
 * @ref ui_main_screen_create call (session only — no NVS persistence).
 *
 * @param theme New theme; silently ignored if >= UI_THEME_COUNT.
 */
void ui_main_screen_set_theme(ui_theme_id_t theme);

/** @brief Return the currently active theme ID. */
ui_theme_id_t ui_main_screen_get_theme(void);

/**
 * @brief Query the display name and gradient colors for a theme entry.
 *
 * @param id       Theme to query; must be < UI_THEME_COUNT.
 * @param name_out Receives a pointer to the static name string; may be NULL.
 * @param top_out  Receives the top gradient color (0xRRGGBB); may be NULL.
 * @param bot_out  Receives the bottom gradient color (0xRRGGBB); may be NULL.
 */
void ui_main_screen_theme_info(ui_theme_id_t id,
                               const char**  name_out,
                               uint32_t*     top_out,
                               uint32_t*     bot_out);

/**
 * @brief Build the main screen on the active LVGL screen for @p display.
 *
 * All interactive widgets share a single @p on_ui_event callback.
 * Distinguish the source inside the callback with:
 *   `ui_main_event_id_t id = (ui_main_event_id_t)(uintptr_t)lv_event_get_user_data(e);`
 *
 * @param display      Active LVGL display.
 * @param on_ui_event  Callback fired for every interactive widget; NULL disables all interactions.
 */
void ui_main_screen_create(lv_display_t* display, lv_event_cb_t on_ui_event);

/**
 * @brief Update digital clock and date labels from formatted strings.
 *
 * @param time_str  Leading "HH:MM:SS" or "HH:MM"; NULL skips time update.
 * @param date_str  e.g. DD.MM.YYYY; NULL skips date update.
 */
void ui_main_screen_update_time(const char* time_str, const char* date_str);

/**
 * @brief Update ambient lux pill (gradient + value marker) and value/threshold labels.
 *
 * @param lux              Current illuminance (lux).
 * @param threshold_lux    Daylight threshold (sub-label + red vertical marker; same semantics as TSL2561).
 * @param display_span_max Bar full scale in lux; use 0 for default auto-scaling base.
 */
void ui_main_screen_update_ambient_lux(uint32_t lux, uint32_t threshold_lux, uint32_t display_span_max);

/**
 * @brief Refresh heater ON/OFF text and pump play/pause icon on the relay panel.
 *
 * @param water_heater_on      Heater relay state for display.
 * @param circulation_pump_on  Pump running state for play vs pause icon.
 */
void ui_main_screen_update_relays(bool water_heater_on, bool circulation_pump_on);

/**
 * @brief Tint the Ethernet status icon (connected vs idle).
 */
void ui_main_screen_update_eth(bool connected);

/**
 * @brief Tint the Wi-Fi status icon (connected vs idle).
 */
void ui_main_screen_update_wifi(bool connected);

/**
 * @brief Tint the MQTT status icon (connected vs idle).
 */
void ui_main_screen_update_mqtt(bool connected);

/**
 * @brief Tint the Bluetooth status icon (connected vs idle).
 */
void ui_main_screen_update_bluetooth(bool connected);

#endif /* __UI_MAIN_SCREEN_H__ */
