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
 * @brief Build the main screen on the active LVGL screen for @p display.
 *
 * @param display               Active LVGL display.
 * @param on_config_pressed     Optional settings-button callback; NULL if unused.
 * @param on_eth_icon_pressed   Optional Ethernet status-icon click; NULL if unused.
 * @param on_wifi_icon_pressed  Optional Wi-Fi status-icon click; NULL if unused.
 */
void ui_main_screen_create(lv_display_t* display,
                           lv_event_cb_t on_config_pressed,
                           lv_event_cb_t on_eth_icon_pressed,
                           lv_event_cb_t on_wifi_icon_pressed);

/**
 * @brief Update digital clock and date labels from formatted strings.
 *
 * @param time_str  Leading "HH:MM:SS" or "HH:MM"; NULL skips time update.
 * @param date_str  e.g. DD.MM.YYYY; NULL skips date update.
 */
void ui_main_screen_update_time(const char* time_str, const char* date_str);

/**
 * @brief Update ambient lux readout, threshold line, and bar scaling.
 *
 * @param lux              Current illuminance (lux).
 * @param threshold_lux    Daylight threshold (same semantics as TSL2561 module).
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
