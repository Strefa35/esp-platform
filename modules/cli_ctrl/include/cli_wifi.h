/**
 * @file cli_wifi.h
 * @brief Wi-Fi CLI commands registration for the console REPL.
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 */

#ifndef __CLI_WIFI_H__
#define __CLI_WIFI_H__

/**
 * @brief Register Wi-Fi console commands (`wifi` …) with esp_console.
 *
 * Call after `esp_console_new_repl_*` (or equivalent) has initialized the console.
 * Built only when `CONFIG_WIFI_CTRL_ENABLE` is set (`cli_wifi.c` is omitted otherwise).
 */
void CliWifi_RegisterConsoleCmd(void);

#endif /* __CLI_WIFI_H__ */
