/**
 * @file mgr_reg.h
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

#include "data.h"
#include "msg.h"


#define MGR_REG_TOPIC_MAX   20
#define MGR_REG_NAME_MAX    20

typedef char mgr_reg_topic_t[MGR_REG_TOPIC_MAX];

typedef char mgr_reg_name_t[MGR_REG_NAME_MAX];

typedef esp_err_t(*mgr_reg_init_f)(void);
typedef esp_err_t(*mgr_reg_done_f)(void);
typedef esp_err_t(*mgr_reg_run_f)(void);
typedef esp_err_t(*mgr_reg_send_f)(const msg_t* msg);

/**
 * Called zero or more times from a module's get_fn to deliver one payload slice.
 * @param payload View of the blob (`type`, `count`, `size`, `data`); `type` should match
 *                the @p kind passed to get_fn. `data` valid only for this call unless documented.
 * @param cb_ctx  Opaque value forwarded from get_fn; use for consumer-specific state (e.g. LCD
 *                list handle vs MQTT JSON buffer). May be NULL.
 * @return        ESP_OK to continue; any other value stops further callbacks and is returned from get_fn.
 */
typedef esp_err_t (*mgr_reg_data_cb_f)(const data_t *payload, void *cb_ctx);

/**
 * Optional bulk read: producer invokes @p cb with a `data_t` view for @p kind, forwarding
 * @p cb_ctx to each call. The same Wi-Fi scan can thus be consumed by LCD, MQTT, etc., each
 * with its own callback and context, without those modules calling each other.
 * NULL if the module does not expose get semantics.
 */
typedef esp_err_t (*mgr_reg_get_f)(data_type_e data_type, mgr_reg_data_cb_f cb, void *cb_ctx);

typedef struct mgr_reg_s {
  mgr_reg_topic_t name; /* name of module */
  uint32_t        type; /* type of module */

  mgr_reg_init_f  init_fn;
  mgr_reg_done_f  done_fn;
  mgr_reg_run_f   run_fn;
  mgr_reg_send_f  send_fn;
  mgr_reg_get_f   get_fn;
} mgr_reg_t;


#endif /* __MGR_REG_H__ */
