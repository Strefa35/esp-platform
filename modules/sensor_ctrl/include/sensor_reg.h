/**
 * @file sensor_reg.h
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor register definition
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#ifndef __SENSOR_REG_H__
#define __SENSOR_REG_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

#include "sensor_data.h"


#define SENSOR_NAME_MAX       20

typedef char sensor_name_t[SENSOR_NAME_MAX];

/**
 * @brief Callback function will be used to notify sensor controler 
 *        about data from the registered sensor
 * @param data - data that the sensor wants to send to the controller
 */
typedef esp_err_t(*sensor_cb_f)(const sensor_data_t* data);

/**
 * @brief Sensor's init function
 * @param cb - callback function for sending data
 */
typedef esp_err_t(*sensor_init_f)(const sensor_cb_f cb);

/**
 * @brief Sensor's done function
 */
typedef esp_err_t(*sensor_done_f)(void);

/**
 * @brief Sensor's run function
 */
typedef esp_err_t(*sensor_run_f)(void);

/**
 * @brief Sensor's set function
 * @param data - data to be set by the controller in the sensor
 */
typedef esp_err_t(*sensor_set_f)(const sensor_data_t* data);

/**
 * @brief Sensor's send function
 * @param data - data to be get by the controller from the sensor
 */
typedef esp_err_t(*sensor_get_f)(sensor_data_t* data);

/**
 * @brief Sensor's register struct
 * 
 */
typedef struct sensor_reg_s {
  sensor_name_t name; /* name of sensor */
  sensor_type_e type; /* type of sensor */
  
  sensor_init_f init;
  sensor_done_f done;
  sensor_run_f  run;
  sensor_set_f  set;
  sensor_get_f  get;

} sensor_reg_t;

#endif /* __SENSOR_REG_H__ */
