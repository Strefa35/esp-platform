/**
 * @file relay_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief Relay Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __RELAY_CTRL_H__
#define __RELAY_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t RelayCtrl_Init(void);
esp_err_t RelayCtrl_Done(void);
esp_err_t RelayCtrl_Run(void);
esp_err_t RelayCtrl_Send(const msg_t* msg);

#endif /* __Relay_CTRL_H__ */