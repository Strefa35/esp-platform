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

typedef struct {
  uint32_t  h;  /* horizontal */
  uint32_t  v;  /* vertical   */
} ns2009_res_t;


typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
} ns2009_touch_t;

esp_err_t ns2009_Init(const ns2009_res_t* res);
esp_err_t ns2009_Done(void);
esp_err_t ns2009_GetTouch(ns2009_touch_t* touch);


#endif /* __NS2009_H__ */