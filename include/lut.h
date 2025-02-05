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

#define GET_ETH_EVENT_NAME(_event) ( \
  _event == ETH_EVENT_START         ? "ETH_EVENT_START"         : \
  _event == ETH_EVENT_STOP          ? "ETH_EVENT_STOP"          : \
  _event == ETH_EVENT_CONNECTED     ? "ETH_EVENT_CONNECTED"     : \
  _event == ETH_EVENT_DISCONNECTED  ? "ETH_EVENT_DISCONNECTED"  : \
                                      "ETH_EVENT_UNKNOWN"         \
)

#define GET_MQTT_EVENT_NAME(_event) ( \
  _event == MQTT_EVENT_ANY              ? "MQTT_EVENT_ANY"            : \
  _event == MQTT_EVENT_ERROR            ? "MQTT_EVENT_ERROR"          : \
  _event == MQTT_EVENT_CONNECTED        ? "MQTT_EVENT_CONNECTED"      : \
  _event == MQTT_EVENT_DISCONNECTED     ? "MQTT_EVENT_DISCONNECTED"   : \
  _event == MQTT_EVENT_SUBSCRIBED       ? "MQTT_EVENT_SUBSCRIBED"     : \
  _event == MQTT_EVENT_UNSUBSCRIBED     ? "MQTT_EVENT_UNSUBSCRIBED"   : \
  _event == MQTT_EVENT_PUBLISHED        ? "MQTT_EVENT_PUBLISHED"      : \
  _event == MQTT_EVENT_DATA             ? "MQTT_EVENT_DATA"           : \
  _event == MQTT_EVENT_BEFORE_CONNECT   ? "MQTT_EVENT_BEFORE_CONNECT" : \
  _event == MQTT_EVENT_DELETED          ? "MQTT_EVENT_DELETED"        : \
  _event == MQTT_USER_EVENT             ? "MQTT_USER_EVENT"           : \
                                          "ETH_EVENT_UNKNOWN"           \
)

#endif /* __LUT_H__ */