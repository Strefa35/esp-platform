/**
 * @file ns2009.h
 * @author A.Czerwinski@pistacje.net
 * @brief Touch Screen Controller Driver
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __NS2009_H__
#define __NS2009_H__


#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

/** Logical display size used to map raw ADC coordinates to screen pixels. */
typedef struct {
  uint32_t h; /**< Horizontal resolution (pixels). */
  uint32_t v; /**< Vertical resolution (pixels). */
} ns2009_res_t;

/** Single touch sample in screen coordinates; @p z is pressure-related raw when available. */
typedef struct {
  int32_t x; /**< X in pixels (0 … res.h-1). */
  int32_t y; /**< Y in pixels (0 … res.v-1). */
  int32_t z; /**< Raw Z1 / pressure channel (device-dependent). */
} ns2009_touch_t;

/**
 * @brief Initialize I2C and the NS2009 controller for the given panel resolution.
 *
 * @param res Non-NULL; both @c h and @c v must be non-zero.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG, or an I2C/driver error.
 */
esp_err_t ns2009_Init(const ns2009_res_t* res);

/**
 * @brief Remove the touch device from the bus and release I2C resources if owned.
 *
 * @return ESP_OK on success.
 */
esp_err_t ns2009_Done(void);

/**
 * @brief Read a touch point mapped to screen coordinates when the panel is pressed.
 *
 * @param touch Out-parameter for coordinates; must be non-NULL.
 * @return ESP_OK when a valid press is detected, ESP_ERR_NOT_FOUND when idle/out of range, or another error.
 */
esp_err_t ns2009_GetTouch(ns2009_touch_t* touch);


#endif /* __NS2009_H__ */