/**
 * @file cfg_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief Configuration Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __CFG_CTRL_H__
#define __CFG_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t CfgCtrl_Init(void);
esp_err_t CfgCtrl_Done(void);
esp_err_t CfgCtrl_Run(void);
esp_err_t CfgCtrl_Send(const msg_t* msg);

#endif /* __CFG_CTRL_H__ */