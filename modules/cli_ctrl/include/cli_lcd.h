/**
 * @file cli_lcd.h
 * @brief LCD ambient console commands for the ESP-IDF REPL.
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 */

#ifndef __CLI_LCD_H__
#define __CLI_LCD_H__

/**
 * @brief Register `lcd` console subcommands (ambient lux / threshold via `MGR_Send` / `MSG_TYPE_LCD_DATA`).
 *
 * Call after `esp_console_new_repl_*`. Built when `CONFIG_LCD_CTRL_ENABLE` and
 * `CONFIG_CLI_CTRL_ENABLE` are set.
 */
void CliLcd_RegisterConsoleCmd(void);

#endif /* __CLI_LCD_H__ */
