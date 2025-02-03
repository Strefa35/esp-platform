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

} msg_type_e;

typedef enum {
  MSG_CTRL_ALL,
  MSG_CTRL_MGR,
  MSG_CTRL_ETH,
  MSG_CTRL_CLI,
  MSG_CTRL_GPIO,
  MSG_CTRL_POWER,
  MSG_CTRL_MQTT,

  MSG_CTRL_TEMPLATE,

} ctrl_type_e;

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
  data_topic_t  topic;
  data_msg_t    msg;
} data_mqtt_t;

/**
 * @brief Message structure
 * 
 */
typedef struct {
  msg_type_e      type;
  ctrl_type_e     from;
  ctrl_type_e     to;
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
