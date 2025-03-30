/**
 * @file sensor_list.h
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor list
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#ifndef __SENSOR_LIST_H__
#define __SENSOR_LIST_H__

#include <stdio.h>
#include <stdbool.h>

#include "sdkconfig.h"

#include "sensor_data.h"
#include "sensor_reg.h"

#ifdef CONFIG_SENSOR_TSL2561_ENABLE
  #include "sensor_tsl2561.h"
#endif


#define SENSOR_LIST_CNT  (sizeof(sensor_list)/sizeof(sensor_reg_t))

static sensor_reg_t sensor_list[] = {

#ifdef CONFIG_SENSOR_TSL2561_ENABLE
  {
    "tsl2561", SENSOR_TYPE_TSL2561,
    sensor_InitTsl2561, sensor_DoneTsl2561, sensor_RunTsl2561, sensor_SetTsl2561, sensor_GetTsl2561
  },
#endif

};

#endif /* __SENSOR_LIST_H__ */
