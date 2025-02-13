/**
 * @file err.h
 * @author A.Czerwinski@pistacje.net
 * @brief Error definitions
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __ERR_H__
#define __ERR_H__

#define ESP_TASK_BASE       0xe000
#define ESP_TASK_INIT       (ESP_TASK_BASE + 1)
#define ESP_TASK_DONE       (ESP_TASK_BASE + 2)
#define ESP_TASK_RUN        (ESP_TASK_BASE + 3)

#endif /* __ERR_H__ */
