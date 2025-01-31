/**
 * @file manager.h
 * @author A.Czerwinski@pistacje.net
 * @brief 
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


esp_err_t MGR_Init(void);
esp_err_t MGR_Run(void);
esp_err_t MGR_Done(void);

#endif /* __MANAGER_H__ */