/**
 * @file tools.h
 * @author A.Czerwinski@pistacje.net
 * @brief Tools
 * @version 0.1
 * @date 2026-02-01
 * 
 * @copyright Copyright (c) 2026 4Embedded.Systems
 * 
 */

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_mac.h"


esp_err_t tools_GetMacAddress(uint8_t *mac_ptr, esp_mac_type_t type);

esp_err_t tools_Init(void);

#endif /* __TOOLS_H__ */
