/**
 * @file cli_ctrl.h
 * @author A.Czerwinski@pistacje.net
 * @brief CLI Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __CLI_CTRL_H__
#define __CLI_CTRL_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

#include "msg.h"

/**
 * @brief Initialize the CLI controller (message queue and worker task).
 */
esp_err_t CliCtrl_Init(void);

/**
 * @brief Shut down the CLI controller, console REPL (if enabled), and worker task.
 */
esp_err_t CliCtrl_Done(void);

/**
 * @brief Start the interactive console REPL when enabled in Kconfig (`CONFIG_CLI_CTRL_ENABLE`).
 */
esp_err_t CliCtrl_Run(void);

/**
 * @brief Post a control message to the CLI module's worker queue.
 *
 * @param msg Message to enqueue; must not be NULL.
 */
esp_err_t CliCtrl_Send(const msg_t *msg);

#endif /* __CLI_CTRL_H__ */