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

esp_err_t tsl2561_InitSensor(void);

#endif /* __SENSOR_TSL2561_H__ */