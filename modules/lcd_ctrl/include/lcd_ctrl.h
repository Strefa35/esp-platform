/**
 * @file lcd_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief LCD Controller
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __LCD_CTRL_H__
#define __LCD_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t LcdCtrl_Init(void);
esp_err_t LcdCtrl_Done(void);
esp_err_t LcdCtrl_Run(void);
esp_err_t LcdCtrl_Send(const msg_t* msg);

#endif /* __LCD_CTRL_H__ */