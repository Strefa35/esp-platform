/**
 * @file mgr_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief Manager Controller
 * @version 0.1
 * @date 2025-01-28
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

#include "mgr_reg.h"

esp_err_t MGR_Init(void);
esp_err_t MGR_Run(void);
esp_err_t MGR_Done(void);
esp_err_t MGR_Send(const msg_t* msg);

/**
 * Dispatch bulk data read. @p module_type must be exactly one `REG_*_CTRL` bit (e.g.
 * `REG_MQTT_CTRL`); otherwise `ESP_ERR_INVALID_ARG`. Finds the first registered entry whose
 * `type` overlaps that bit; if `get_fn` is set, calls `get_fn(kind, cb, cb_ctx)` (the module
 * invokes @p cb zero or more times). `ESP_ERR_NOT_SUPPORTED` if the module matches but has no
 * `get_fn`; `ESP_ERR_NOT_FOUND` if no entry matches.
 */
esp_err_t MGR_GetData(uint32_t module_type, data_type_e data_type, mgr_reg_data_cb_f cb, void *cb_ctx);

#endif /* __MANAGER_H__ */
