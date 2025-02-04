/**
 * @file mqtt_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief MQTT Controller
 * @version 0.1
 * @date 2025-02-03
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __MQTT_CTRL_H__
#define __MQTT_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t MqttCtrl_Init(void);
esp_err_t MqttCtrl_Done(void);
esp_err_t MqttCtrl_Run(void);
esp_err_t MqttCtrl_Send(const msg_t* msg);

#endif /* __MQTT_CTRL_H__ */