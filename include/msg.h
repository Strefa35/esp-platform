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


/*
==================================================================
  ENUMS
==================================================================
*/

/* Message type definition */
typedef enum {
  /* MGR module */
  MSG_TYPE_MGR_LIST,

  /* ETH module */
  MSG_TYPE_ETH_EVENT,
  MSG_TYPE_ETH_MAC,
  MSG_TYPE_ETH_IP,

  /* MQTT module */
  MSG_TYPE_MQTT_START,
  MSG_TYPE_MQTT_EVENT,
  MSG_TYPE_MQTT_DATA,
  MSG_TYPE_MQTT_PUBLISH,
  MSG_TYPE_MQTT_SUBSCRIBE,

} msg_type_e;

/* ETH state definition */
typedef enum {
  DATA_ETH_EVENT_START,
  DATA_ETH_EVENT_STOP,
  DATA_ETH_EVENT_CONNECTED,
  DATA_ETH_EVENT_DISCONNECTED,
} data_eth_event_e;

/* MQTT state definition */
typedef enum {
  DATA_MQTT_EVENT_ANY,
  DATA_MQTT_EVENT_ERROR,
  DATA_MQTT_EVENT_CONNECTED,
  DATA_MQTT_EVENT_DISCONNECTED,
  DATA_MQTT_EVENT_SUBSCRIBED,
  DATA_MQTT_EVENT_UNSUBSCRIBED,
  DATA_MQTT_EVENT_PUBLISHED,
  DATA_MQTT_EVENT_DATA,
  DATA_MQTT_EVENT_BEFORE_CONNECT,
  DATA_MQTT_EVENT_DELETED,
  DATA_MQTT_EVENT_USER,
} data_mqtt_event_e;


/*
==================================================================
  Bits definition for 'from/to'
==================================================================
*/

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


/*
==================================================================
  Message structure
==================================================================
*/


#define DATA_TOPIC_SIZE     (20U)
#define DATA_MSG_SIZE       (100U)
#define DATA_JSON_SIZE      (150U)


typedef char data_topic_t[DATA_TOPIC_SIZE];
typedef char data_msg_t[DATA_MSG_SIZE];
typedef char data_json_t[DATA_JSON_SIZE];

/* ETH MAC definition */
typedef uint8_t data_eth_mac_t[6];

/* ETH IP definition */
typedef struct {
  uint32_t  ip;
  uint32_t  mask;
  uint32_t  gw;
} data_eth_info_t;

/* ETH message payload */
typedef struct {
  union {
    data_eth_event_e  event_id;
    data_eth_mac_t    mac;
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

/* MQTT data definition */
typedef struct {
  data_topic_t  topic;
  data_msg_t    msg;
} data_mqtt_data_t;

/* MQTT message payload */
typedef struct {
  union {
    data_mqtt_event_e event_id;
    data_mqtt_data_t  data;
    data_json_t       json;
  } u;
} payload_mqtt_t;

/* Error message structure */
typedef struct {

} payload_error_t;

/**
 * @brief Message structure
 * 
 */
typedef struct {
  msg_type_e      type;
  uint32_t        from;
  uint32_t        to;
  union {
    payload_eth_t     eth;
    payload_cli_t     cli;
    payload_gpio_t    gpio;
    payload_power_t   power;
    payload_mqtt_t    mqtt;
    payload_error_t   error;
  } payload;
} msg_t;


#endif /* __MSG_H__ */
