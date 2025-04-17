/**
 * @file nvs_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief NVS Controller
 * @version 0.1
 * @date 2025-04-15
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __NVS_CTRL_H__
#define __NVS_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

struct nvs_s;
typedef struct nvs_s* nvs_t;

esp_err_t NVS_Open(const char* partition, nvs_t* const handle_ptr);
esp_err_t NVS_Close(nvs_t handle);
esp_err_t NVS_Read(const nvs_t handle, const char *key, void* data_ptr, size_t* data_size);
esp_err_t NVS_Write(const nvs_t handle, const char *key, void* data_ptr, size_t data_size);

esp_err_t NVS_Init(void);
esp_err_t NVS_Done(void);

#endif /* __NVS_CTRL_H__ */