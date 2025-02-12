/**
 * @file eth_lut.h
 * @author A.Czerwinski@pistacje.net
 * @brief Look Up Table
 * @version 0.1
 * @date 2025-02-11
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __ETH_LUT_H__
#define __ETH_LUT_H__


#define GET_ETHERNET_EVENT_NAME(_event) ( \
  _event == ETHERNET_EVENT_START              ? "ETHERNET_EVENT_START"          : \
  _event == ETHERNET_EVENT_STOP               ? "ETHERNET_EVENT_STOP"           : \
  _event == ETHERNET_EVENT_CONNECTED          ? "ETHERNET_EVENT_CONNECTED"      : \
  _event == ETHERNET_EVENT_DISCONNECTED       ? "ETHERNET_EVENT_DISCONNECTED"   : \
                                                "ETHERNET_EVENT_UNKNOWN"          \
)

#endif /* __ETH_LUT_H__ */