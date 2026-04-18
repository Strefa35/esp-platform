/**
 * @file cli_lcd.c
 * @brief Console commands to push ambient lux and threshold to the LCD module via `MGR_Send` (`MSG_TYPE_LCD_DATA`).
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sdkconfig.h"

#if CONFIG_CLI_CTRL_ENABLE && CONFIG_LCD_CTRL_ENABLE

#include "esp_console.h"

#include "cli_lcd.h"
#include "mgr_ctrl.h"
#include "msg.h"

/** Keep in sync with `LCD_MASK_AMBIENT_*` in `lcd_helper.h`. */
#define CLI_LCD_MASK_AMBIENT_LUX        (1u << 20)
#define CLI_LCD_MASK_AMBIENT_THRESHOLD  (1u << 21)

static bool parse_u32(const char* s, uint32_t* out) {
  if (s == NULL || out == NULL) {
    return false;
  }
  char* end = NULL;
  errno     = 0;
  unsigned long v = strtoul(s, &end, 0);
  if (end == s || *end != '\0' || errno == ERANGE || v > (unsigned long)UINT32_MAX) {
    return false;
  }
  *out = (uint32_t)v;
  return true;
}

/**
 * @brief Console handler for the `lcd` command (lux, threshold, ambient).
 */
static int clicmd_lcd(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage:\n");
    printf("  lcd lux <lux>\n");
    printf("  lcd threshold <threshold_lux>\n");
    printf("  lcd ambient <lux> <threshold_lux>\n");
    return 1;
  }

  if (strcmp(argv[1], "lux") == 0) {
    if (argc != 3) {
      printf("Usage: lcd lux <lux>\n");
      return 1;
    }
    uint32_t lux = 0;
    if (!parse_u32(argv[2], &lux)) {
      printf("Invalid lux value\n");
      return 1;
    }
    msg_t msg = {
      .type = MSG_TYPE_LCD_DATA,
      .from = REG_CLI_CTRL,
      .to   = REG_LCD_CTRL,
      .payload.lcd.mask        = CLI_LCD_MASK_AMBIENT_LUX,
      .payload.lcd.d_uint32[0] = lux,
    };
    if (MGR_Send(&msg) != ESP_OK) {
      printf("MGR_Send(lcd lux) failed\n");
      return 1;
    }
    printf("LCD lux update sent (applied on next UI tick)\n");
    return 0;
  }

  if (strcmp(argv[1], "threshold") == 0) {
    if (argc != 3) {
      printf("Usage: lcd threshold <threshold_lux>\n");
      return 1;
    }
    uint32_t th = 0;
    if (!parse_u32(argv[2], &th)) {
      printf("Invalid threshold value\n");
      return 1;
    }
    msg_t msg = {
      .type = MSG_TYPE_LCD_DATA,
      .from = REG_CLI_CTRL,
      .to   = REG_LCD_CTRL,
      .payload.lcd.mask        = CLI_LCD_MASK_AMBIENT_THRESHOLD,
      .payload.lcd.d_uint32[1] = th,
    };
    if (MGR_Send(&msg) != ESP_OK) {
      printf("MGR_Send(lcd threshold) failed\n");
      return 1;
    }
    printf("LCD threshold update sent (applied on next UI tick)\n");
    return 0;
  }

  if (strcmp(argv[1], "ambient") == 0) {
    if (argc != 4) {
      printf("Usage: lcd ambient <lux> <threshold_lux>\n");
      return 1;
    }
    uint32_t lux = 0;
    uint32_t th  = 0;
    if (!parse_u32(argv[2], &lux) || !parse_u32(argv[3], &th)) {
      printf("Invalid lux or threshold value\n");
      return 1;
    }
    msg_t msg = {
      .type = MSG_TYPE_LCD_DATA,
      .from = REG_CLI_CTRL,
      .to   = REG_LCD_CTRL,
      .payload.lcd.mask        = CLI_LCD_MASK_AMBIENT_LUX | CLI_LCD_MASK_AMBIENT_THRESHOLD,
      .payload.lcd.d_uint32[0] = lux,
      .payload.lcd.d_uint32[1] = th,
    };
    if (MGR_Send(&msg) != ESP_OK) {
      printf("MGR_Send(lcd ambient) failed\n");
      return 1;
    }
    printf("LCD ambient update sent (applied on next UI tick)\n");
    return 0;
  }

  printf("Unknown lcd subcommand: %s\n", argv[1]);
  return 1;
}

void CliLcd_RegisterConsoleCmd(void) {
  const esp_console_cmd_t cmd = {
    .command = "lcd",
    .help    = "lcd lux <lux> | lcd threshold <lux> | lcd ambient <lux> <threshold_lux>",
    .hint    = NULL,
    .func    = &clicmd_lcd,
  };
  (void)esp_console_cmd_register(&cmd);
}

#endif /* CONFIG_CLI_CTRL_ENABLE && CONFIG_LCD_CTRL_ENABLE */
