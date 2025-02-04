/**
 * @file template_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief Template Controller
 * @version 0.1
 * @date 2025-01-31
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __TEMPLATE_CTRL_H__
#define __TEMPLATE_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t TemplateCtrl_Init(void);
esp_err_t TemplateCtrl_Done(void);
esp_err_t TemplateCtrl_Run(void);
esp_err_t TemplateCtrl_Send(const msg_t* msg);

#endif /* __TEMPLATE_CTRL_H__ */