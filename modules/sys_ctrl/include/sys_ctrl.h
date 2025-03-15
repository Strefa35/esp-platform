/**
 * @file sys_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief System Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __SYS_CTRL_H__
#define __SYS_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t SysCtrl_Init(void);
esp_err_t SysCtrl_Done(void);
esp_err_t SysCtrl_Run(void);
esp_err_t SysCtrl_Send(const msg_t* msg);

#endif /* __SYS_CTRL_H__ */