/**
 * @file lcd_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief LCD Controller
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __LCD_CTRL_H__
#define __LCD_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"
#include "msg.h"

/**
 * @brief Initialize LCD helper, message queue, and the LCD controller task.
 *
 * @return ESP_OK on success, or an error from the helper or FreeRTOS setup.
 */
esp_err_t LcdCtrl_Init(void);

/**
 * @brief Stop the controller task and release helper resources.
 *
 * @return ESP_OK on success, or an error from teardown.
 */
esp_err_t LcdCtrl_Done(void);

/**
 * @brief Run hook for the manager lifecycle (currently no extra runtime work).
 *
 * @return ESP_OK.
 */
esp_err_t LcdCtrl_Run(void);

/**
 * @brief Enqueue a message for the LCD controller task (non-blocking send).
 *
 * @param msg Message to copy into the internal queue; must remain valid until copied.
 * @return ESP_OK if queued, ESP_FAIL if the queue is full.
 */
esp_err_t LcdCtrl_Send(const msg_t* msg);

#endif /* __LCD_CTRL_H__ */
