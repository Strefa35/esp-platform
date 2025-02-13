/**
 * @file cli_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief CLI Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __CLI_CTRL_H__
#define __CLI_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t CliCtrl_Init(void);
esp_err_t CliCtrl_Done(void);
esp_err_t CliCtrl_Run(void);
esp_err_t CliCtrl_Send(const msg_t* msg);

#endif /* __CLI_CTRL_H__ */