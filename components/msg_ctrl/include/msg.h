/**
 * @file msg.h
 * @author A.Czerwinski@pistacje.net
 * @brief 
 * @version 0.1
 * @date 2025-01-28
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __MSG_H__
#define __MSG_H__

#include "esp_err.h"


#define DATA_TOPIC_SIZE     20
#define DATA_MSG_SIZE       100

typedef char data_topic_t[DATA_TOPIC_SIZE];
typedef char data_msg_t[DATA_MSG_SIZE];

typedef enum {
  MSG_TYPE_ETH_EVENT,
  MSG_TYPE_ETH_IP,

  MSG_TYPE_MQTT_EVENT,
  MSG_TYPE_MQTT_DATA,

} msg_type_e;

/* ----------[ALL bits]----------- */
#define MSG_ALL_CTRL    (~0)
/* ----------[4 bits]------------- */
#define MSG_MGR_CTRL    (1 << 0)
#define MSG_ETH_CTRL    (1 << 1)
#define MSG_CLI_CTRL    (1 << 2)
#define MSG_GPIO_CTRL   (1 << 3)
/* ----------[4 bits]------------- */
#define MSG_POWER_CTRL  (1 << 4)
#define MSG_MQTT_CTRL   (1 << 5)
/* ----------[END]---------------- */

typedef struct {

} data_mgr_t;

typedef struct {
} data_eth_event_t;

typedef struct {
  char  event[20];
  char  ip[20];
  char  mask[20];
  char  gw[20];
} data_eth_t;

typedef struct {

} data_cli_t;

typedef struct {

} data_gpio_t;

typedef struct {

} data_power_t;

typedef struct {
  int32_t       event_id;
  data_topic_t  topic;
  data_msg_t    msg;
} data_mqtt_t;

/**
 * @brief Message structure
 * 
 */
typedef struct {
  msg_type_e      type;
  uint32_t        from;
  uint32_t        to;
  union {
    data_mgr_t    mgr;
    data_eth_t    eth;
    data_cli_t    cli;
    data_gpio_t   gpio;
    data_power_t  power;
    data_mqtt_t   mqtt;
  } data;
} msg_t;


#endif /* __MSG_H__ */
