/**
 * @file lut.h
 * @author A.Czerwinski@pistacje.net
 * @brief Look Up Table
 * @version 0.1
 * @date 2025-02-04
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __LUT_H__
#define __LUT_H__

#include "msg.h"

#define GET_MSG_TYPE_NAME(_type) ( \
  _type == MSG_TYPE_MGR_LIST                  ? "MSG_TYPE_MGR_LIST"               : \
  _type == MSG_TYPE_ETH_EVENT                 ? "MSG_TYPE_ETH_EVENT"              : \
  _type == MSG_TYPE_ETH_IP                    ? "MSG_TYPE_ETH_IP"                 : \
  _type == MSG_TYPE_MQTT_EVENT                ? "MSG_TYPE_MQTT_EVENT"             : \
  _type == MSG_TYPE_MQTT_DATA                 ? "MSG_TYPE_MQTT_DATA"              : \
                                                "MSG_TYPE_UNKNOWN"                  \
)

#define GET_DATA_ETH_EVENT_NAME(_event) ( \
  _event == DATA_ETH_EVENT_START              ? "DATA_ETH_EVENT_START"            : \
  _event == DATA_ETH_EVENT_STOP               ? "DATA_ETH_EVENT_STOP"             : \
  _event == DATA_ETH_EVENT_CONNECTED          ? "DATA_ETH_EVENT_CONNECTED"        : \
  _event == DATA_ETH_EVENT_DISCONNECTED       ? "DATA_ETH_EVENT_DISCONNECTED"     : \
                                                "DATA_ETH_EVENT_UNKNOWN"            \
)

#define GET_DATA_MQTT_EVENT_NAME(_event) ( \
  _event == DATA_MQTT_EVENT_ANY               ? "DATA_MQTT_EVENT_ANY"             : \
  _event == DATA_MQTT_EVENT_ERROR             ? "DATA_MQTT_EVENT_ERROR"           : \
  _event == DATA_MQTT_EVENT_CONNECTED         ? "DATA_MQTT_EVENT_CONNECTED"       : \
  _event == DATA_MQTT_EVENT_DISCONNECTED      ? "DATA_MQTT_EVENT_DISCONNECTED"    : \
  _event == DATA_MQTT_EVENT_SUBSCRIBED        ? "DATA_MQTT_EVENT_SUBSCRIBED"      : \
  _event == DATA_MQTT_EVENT_UNSUBSCRIBED      ? "DATA_MQTT_EVENT_UNSUBSCRIBED"    : \
  _event == DATA_MQTT_EVENT_PUBLISHED         ? "DATA_MQTT_EVENT_PUBLISHED"       : \
  _event == DATA_MQTT_EVENT_DATA              ? "DATA_MQTT_EVENT_DATA"            : \
  _event == DATA_MQTT_EVENT_BEFORE_CONNECT    ? "DATA_MQTT_EVENT_BEFORE_CONNECT"  : \
  _event == DATA_MQTT_EVENT_DELETED           ? "DATA_MQTT_EVENT_DELETED"         : \
  _event == DATA_MQTT_EVENT_USER              ? "DATA_MQTT_EVENT_USER"            : \
                                                "DATA_MQTT_EVENT_UNKNOWN"           \
)

#endif /* __LUT_H__ */