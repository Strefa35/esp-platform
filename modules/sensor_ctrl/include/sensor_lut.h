/**
 * @file sensor_lut.h
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor LUT
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#ifndef __SENSOR_LUT_H__
#define __SENSOR_LUT_H__

#include "sensor_data.h"


#define GET_SENSOR_TYPE_NAME(_type) ( \
  _type == SENSOR_TYPE_TSL2561      ? "SENSOR_TYPE_TSL2561"     : \
  _type == SENSOR_TYPE_MAX          ? "SENSOR_TYPE_MAX"         : \
                                      "SECTOR_TYPE_UNKNOWN"       \
)

#define GET_SENSOR_DATA_NAME(_dtype) ( \
  _dtype == SENSOR_DATA_THERSHOLD   ? "SENSOR_DATA_THERSHOLD"   : \
  _dtype == SENSOR_DATA_LUX         ? "SENSOR_DATA_LUX"         : \
  _dtype == SENSOR_DATA_MAX         ? "SENSOR_DATA_MAX"         : \
                                      "SENSOR_DATA_UNKNOWN"       \
)

#endif /* __SENSOR_LUT_H__ */
