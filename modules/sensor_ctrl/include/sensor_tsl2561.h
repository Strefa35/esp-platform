/**
 * @file sensor_tsl2561.h
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor TSL2561 
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __SENSOR_TSL2561_H__
#define __SENSOR_TSL2561_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

#include "sensor_data.h"
#include "sensor_reg.h"


esp_err_t sensor_InitTsl2561(const sensor_cb_f cb);
esp_err_t sensor_DoneTsl2561(void);
esp_err_t sensor_RunTsl2561(void);
esp_err_t sensor_SetTsl2561(const sensor_data_t* data);
esp_err_t sensor_GetTsl2561(sensor_data_t* data);


#endif /* __SENSOR_TSL2561_H__ */