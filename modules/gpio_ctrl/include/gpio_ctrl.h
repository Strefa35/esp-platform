/**
 * @file gpio_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief GPIO Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __GPIO_CTRL_H__
#define __GPIO_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t GpioCtrl_Init(void);
esp_err_t GpioCtrl_Done(void);
esp_err_t GpioCtrl_Run(void);
esp_err_t GpioCtrl_Send(const msg_t* msg);

#endif /* __GPIO_CTRL_H__ */