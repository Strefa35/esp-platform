/**
 * @file wifi_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief Wi-Fi STA controller (scan, connect, events to the manager)
 * @version 0.1
 * @date 2026-04-03
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 *
 */

#ifndef __WIFI_CTRL_H__
#define __WIFI_CTRL_H__

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t WifiCtrl_Init(void);
esp_err_t WifiCtrl_Done(void);
esp_err_t WifiCtrl_Run(void);
esp_err_t WifiCtrl_Send(const msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_CTRL_H__ */
