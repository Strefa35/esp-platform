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
  /* MGR module */
  MSG_TYPE_MGR_LIST,

  /* ETH module */
  MSG_TYPE_ETH_EVENT,
  MSG_TYPE_ETH_IP,

  /* MQTT module */
  MSG_TYPE_MQTT_EVENT,
  MSG_TYPE_MQTT_DATA,

} msg_type_e;

/* ----------[ALL bits]----------- */
#define REG_ALL_CTRL    (~0)

/* [1st byte]==========[8 bits]============= */
#define REG_MGR_CTRL    (1 << 0)
#define REG_ETH_CTRL    (1 << 1)
#define REG_CLI_CTRL    (1 << 2)
#define REG_GPIO_CTRL   (1 << 3)
/* ----------[4 bits]------------- */
#define REG_POWER_CTRL  (1 << 4)
#define REG_MQTT_CTRL   (1 << 5)

/* [2nd byte]==========[8 bits]============= */

/* [3rd byte]==========[8 bits]============= */

/* [4th byte]==========[8 bits]============= */
/*   Control bits                            */

/* Can't communicate with them from the outside */
#define REG_INT_CTRL    (1 << 30)   /* Internal Controller  */


/* ----------[END]---------------- */

/* MGR message payload */
typedef struct {
  data_msg_t  msg;
} payload_mgr_t;

/* ETH state definition */
typedef enum {
  ETH_EVENT_START,
  ETH_EVENT_STOP,
  ETH_EVENT_CONNECTED,
  ETH_EVENT_DISCONNECTED,
} data_eth_event_e;

/* ETH MAC definition */
typedef uint8_t data_eth_mac_t[6];

/* ETH event definition */
typedef struct {
  data_eth_event_e  id;
  data_eth_mac_t    mac;
} data_eth_event_t;

/* ETH IP definition */
typedef struct {
  uint32_t  ip;
  uint32_t  mask;
  uint32_t  gw;
} data_eth_info_t;

/* ETH message payload */
typedef struct {
  union {
    data_eth_event_t  event;
    data_eth_info_t   info;
  } u;
} payload_eth_t;

/* CLI message payload */
typedef struct {

} payload_cli_t;

/* GPIO message payload */
typedef struct {

} payload_gpio_t;

/* POWER message payload */
typedef struct {

} payload_power_t;

/* MQTT message payload */
typedef struct {
  int32_t       event_id;
  data_topic_t  topic;
  data_msg_t    msg;
} payload_mqtt_t;

/**
 * @brief Message structure
 * 
 */
typedef struct {
  msg_type_e      type;
  uint32_t        from;
  uint32_t        to;
  union {
    payload_mgr_t     mgr;
    payload_eth_t     eth;
    payload_cli_t     cli;
    payload_gpio_t    gpio;
    payload_power_t   power;
    payload_mqtt_t    mqtt;
  } payload;
} msg_t;


#endif /* __MSG_H__ */
