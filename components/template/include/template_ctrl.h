/**
 * @file eth_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief Eth Controller
 * @version 0.1
 * @date 2025-01-31
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __ETH_CTRL_H__
#define __ETH_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t EthCtrl_Init(void);
esp_err_t EthCtrl_Done(void);
esp_err_t EthCtrl_Run(void);
esp_err_t EthCtrl_Send(const char* msg);

#endif /* __ETH_CTRL_H__ */