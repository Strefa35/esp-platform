/**
 * @file mgr-reg.h
 * @author A.Czerwinski@pistacje.net
 * @brief 
 * @version 0.1
 * @date 2025-01-30
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#ifndef __MGR_REG_H__
#define __MGR_REG_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

#include "msg.h"


#define MGR_REG_TOPIC_MAX   20

typedef char mgr_reg_topic_t[MGR_REG_TOPIC_MAX];

typedef esp_err_t(*mgr_reg_init_f)(void);
typedef esp_err_t(*mgr_reg_done_f)(void);
typedef esp_err_t(*mgr_reg_run_f)(void);
typedef esp_err_t(*mgr_reg_send_f)(const msg_t* msg);

typedef struct mgr_reg_s {
  mgr_reg_topic_t topic;
  mgr_reg_init_f  init_fn;
  mgr_reg_done_f  done_fn;
  mgr_reg_run_f   run_fn;
  mgr_reg_send_f  send_fn;
} mgr_reg_t;


#endif /* __MGR_REG_H__ */
