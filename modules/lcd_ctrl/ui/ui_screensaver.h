/**
 * @file ui_screensaver.h
 * @author A.Czerwinski@pistacje.net
 * @brief Screensaver UI - shows only clock and date
 * @version 0.1
 * @date 2026-03-23
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 *
 */

#ifndef __UI_SCREENSAVER_H__
#define __UI_SCREENSAVER_H__

#include "lvgl.h"

/**
 * @brief Create and render the screensaver screen onto the active LVGL screen.
 *
 * @param display Active LVGL display handle.
 */
void ui_screensaver_create(lv_display_t* display);

/**
 * @brief Update analog clock hands and date label.
 *
 * @param time_str Time string, e.g. "13:45:22" (must not be NULL)
 * @param date_str Date string, e.g. "Mon, 23 Mar 2026" (NULL keeps previous value)
 */
void ui_screensaver_update_time(const char* time_str, const char* date_str);

/**
 * @brief Update weather data displayed on the screensaver.
 *
 * @param temperature_str Temperature string, e.g. "21 C" (NULL keeps previous value)
 * @param summary_str Weather summary, e.g. "Partly cloudy" (NULL keeps previous value)
 * @param location_str Location string, e.g. "Krakow" (NULL keeps previous value)
 */
void ui_screensaver_update_weather(const char* temperature_str,
								   const char* summary_str,
								   const char* location_str);

#endif /* __UI_SCREENSAVER_H__ */
