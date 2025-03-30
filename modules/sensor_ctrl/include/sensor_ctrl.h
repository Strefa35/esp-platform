/**
 * @file sensor_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor Controller
 * @version 0.1
 * @date 2025-03-24
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __SENSOR_CTRL_H__
#define __SENSOR_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

#include "msg.h"

esp_err_t SensorCtrl_Init(void);
esp_err_t SensorCtrl_Done(void);
esp_err_t SensorCtrl_Run(void);
esp_err_t SensorCtrl_Send(const msg_t* msg);

#endif /* __SENSOR_CTRL_H__ */