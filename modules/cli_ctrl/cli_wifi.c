/**
 * @file cli_wifi.c
 * @brief Console commands for the Wi-Fi module (`wifi scan|connect|disconnect`).
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "esp_console.h"

#include "cli_wifi.h"
#include "mgr_ctrl.h"
#include "msg.h"

/**
 * @brief Console handler for the `wifi` command (scan, connect, disconnect).
 */
static int clicmd_wifi(int argc, char **argv)
{
  if (argc < 2) {
    printf("Usage: wifi scan | wifi connect <ssid> <password> [authmode] | wifi disconnect\n");
    return 1;
  }

  if (strcmp(argv[1], "scan") == 0) {
    msg_t msg = {
      .type = MSG_TYPE_WIFI_SCAN_REQ,
      .from = REG_CLI_CTRL,
      .to   = REG_WIFI_CTRL,
    };
    if (MGR_Send(&msg) != ESP_OK) {
      printf("MGR_Send(scan) failed\n");
      return 1;
    }
    printf("Scan requested (see ESP::WIFI logs for SSID/RSSI)\n");
    return 0;
  }

  if (strcmp(argv[1], "disconnect") == 0) {
    msg_t msg = {
      .type = MSG_TYPE_WIFI_DISCONNECT,
      .from = REG_CLI_CTRL,
      .to   = REG_WIFI_CTRL,
    };
    if (MGR_Send(&msg) != ESP_OK) {
      printf("MGR_Send(disconnect) failed\n");
      return 1;
    }
    printf("Disconnect requested\n");
    return 0;
  }

  if (strcmp(argv[1], "connect") == 0) {
    if (argc < 4) {
      printf("Usage: wifi connect <ssid> <password> [authmode]\n");
      printf("authmode: wifi_auth_mode_t value (e.g. 3=WPA2_PSK); omit or 0 for OPEN\n");
      return 1;
    }

    msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_TYPE_WIFI_CONNECT;
    msg.from = REG_CLI_CTRL;
    msg.to   = REG_WIFI_CTRL;

    strncpy(msg.payload.wifi.u.connect.ssid, argv[2], sizeof(msg.payload.wifi.u.connect.ssid) - 1U);
    msg.payload.wifi.u.connect.ssid[sizeof(msg.payload.wifi.u.connect.ssid) - 1U] = '\0';
    strncpy(msg.payload.wifi.u.connect.password, argv[3], sizeof(msg.payload.wifi.u.connect.password) - 1U);
    msg.payload.wifi.u.connect.password[sizeof(msg.payload.wifi.u.connect.password) - 1U] = '\0';

    msg.payload.wifi.u.connect.channel = 0;
    if (argc >= 5) {
      unsigned long a = strtoul(argv[4], NULL, 0);
      if (a > 255UL) {
        printf("authmode out of range\n");
        return 1;
      }
      msg.payload.wifi.u.connect.authmode = (uint8_t)a;
    }

    if (MGR_Send(&msg) != ESP_OK) {
      printf("MGR_Send(connect) failed\n");
      return 1;
    }
    printf("Connect requested for SSID \"%s\"\n", msg.payload.wifi.u.connect.ssid);
    return 0;
  }

  printf("Unknown wifi subcommand: %s\n", argv[1]);
  return 1;
}

void CliWifi_RegisterConsoleCmd(void)
{
  const esp_console_cmd_t cmd = {
    .command = "wifi",
    .help    = "wifi scan | wifi connect <ssid> <pass> [authmode] | wifi disconnect",
    .hint    = NULL,
    .func    = &clicmd_wifi,
  };
  (void)esp_console_cmd_register(&cmd);
}
