/**
 * @file sensor_data.h
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor data
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#ifndef __SENSOR_DATA_H__
#define __SENSOR_DATA_H__

#include <stdio.h>
#include <stdbool.h>


typedef enum {
  SENSOR_TYPE_TSL2561,

  SENSOR_TYPE_MAX
} sensor_type_e;

typedef enum {
  SENSOR_DATA_INFO,
  SENSOR_DATA_THERSHOLD,
  SENSOR_DATA_LUX,

  SENSOR_DATA_MAX
} sensor_data_e;


typedef struct {
  sensor_type_e type;
  sensor_data_e dtype;

  union {
    uint8_t   uint8[8];
    int8_t    int8[8];

    uint16_t  uint16[4];
    int16_t   int16[4];
    
    uint32_t  uint32[2];
    uint32_t  int32[2];

    float     ffloat[2];
  } u;

} sensor_data_t;

#endif /* __SENSOR_DATA_H__ */
