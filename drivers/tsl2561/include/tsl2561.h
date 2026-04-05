/**
 * @file tsl2561.h
 * @author A.Czerwinski@pistacje.net
 * @brief Light to digital converter for TSL2560 & TSL2561
 * @version 0.1
 * @date 2025-03-16
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __TSL2561_H__
#define __TSL2561_H__


#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"


typedef enum {
  GAIN_1X   = 0x00,
  GAIN_10X  = 0x10
} tsl2561_gain_e;

typedef enum {
  INTEG_13MS  = 0,
  INTEG_101MS,
  INTEG_402MS,
  INTEG_NA
} tsl2561_integ_e;

typedef enum {
  CTRL_ISR_CLEAR,
  CTRL_ISR_DISABLE,
  CTRL_ISR_ENABLE,
} tsl2561_ctrl_e;

typedef struct {
  uint16_t  min;
  uint16_t  max;
} tsl2561_threshold_t;


struct tsl2561_s;
typedef struct tsl2561_s* tsl2561_t;

typedef void(*tsl2561_isr_f)(tsl2561_t handle);

esp_err_t tsl2561_SetPower(const tsl2561_t handle, const bool on);
esp_err_t tsl2561_GetPower(const tsl2561_t handle, bool* on);

esp_err_t tsl2561_SetGain(const tsl2561_t handle, const tsl2561_gain_e gain);
esp_err_t tsl2561_SetIntegrationTime(const tsl2561_t handle, const tsl2561_integ_e time);
esp_err_t tsl2561_GetLux(const tsl2561_t handle, uint32_t* lux);
esp_err_t tsl2561_GetLight(const tsl2561_t handle, uint16_t* broadband, uint16_t* infrared, uint32_t* lux, float* ratio);

esp_err_t tsl2561_SetIsrNotify(const tsl2561_t handle, const tsl2561_isr_f fn);
esp_err_t tsl2561_SetIsrThreshold(const tsl2561_t handle, const tsl2561_threshold_t* threshold);
esp_err_t tsl2561_SetIsrControl(const tsl2561_t handle, const tsl2561_ctrl_e ctrl);

esp_err_t tsl2561_GetId(const tsl2561_t handle, uint8_t* id);

esp_err_t tsl2561_InitIsr(const tsl2561_t handle);
esp_err_t tsl2561_DoneIsr(const tsl2561_t handle);

esp_err_t tsl2561_Init(tsl2561_t* const handle);
esp_err_t tsl2561_Done(tsl2561_t handle);


#endif /* __TSL2561_H__ */