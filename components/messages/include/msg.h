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


#define MSG_TOPIC_SIZE      20
#define MSG_PAYLOAD_SIZE    100

typedef char msg_topic_t[MSG_TOPIC_SIZE];
typedef char msg_payload_t[MSG_PAYLOAD_SIZE];

typedef enum {
  MSG_TYPE_ETH,
  MSG_TYPE_CLI,
  MSG_TYPE_GPIO,
  MSG_TYPE_POWER,
  MSG_TYPE_MQTT,

  MSG_TYPE_TEMPLATE,

} msg_type_e;

typedef struct {
  msg_type_e    type;
  msg_topic_t   topic;
  msg_payload_t payload;
} msg_t;


#endif /* __MSG_H__ */
