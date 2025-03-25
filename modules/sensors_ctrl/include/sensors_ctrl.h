/**
 * @file sensors_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief Sensors Controller
 * @version 0.1
 * @date 2025-03-24
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __SENSORS_CTRL_H__
#define __SENSORS_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t SensorsCtrl_Init(void);
esp_err_t SensorsCtrl_Done(void);
esp_err_t SensorsCtrl_Run(void);
esp_err_t SensorsCtrl_Send(const msg_t* msg);

#endif /* __SENSORS_CTRL_H__ */