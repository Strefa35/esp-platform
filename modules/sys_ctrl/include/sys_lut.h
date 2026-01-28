/**
 * @file sys_lut.h
 * @author A.Czerwinski@pistacje.net
 * @brief Look Up Table
 * @version 0.1
 * @date 2026-01-27
 * 
 * @copyright Copyright (c) 2026 4Embedded.Systems
 * 
 */

#ifndef __SYS_LUT_H__
#define __SYS_LUT_H__


#define GET_ESP_MAC_TYPE_NAME(_type) ( \
  _type == ESP_MAC_WIFI_STA       ? "ESP_MAC_WIFI_STA"      : \
  _type == ESP_MAC_WIFI_SOFTAP    ? "ESP_MAC_WIFI_SOFTAP"   : \
  _type == ESP_MAC_BT             ? "ESP_MAC_BT"            : \
  _type == ESP_MAC_ETH            ? "ESP_MAC_ETH"           : \
  _type == ESP_MAC_IEEE802154     ? "ESP_MAC_IEEE802154"    : \
  _type == ESP_MAC_BASE           ? "ESP_MAC_BASE"          : \
  _type == ESP_MAC_EFUSE_FACTORY  ? "ESP_MAC_EFUSE_FACTORY" : \
  _type == ESP_MAC_EFUSE_CUSTOM   ? "ESP_MAC_EFUSE_CUSTOM"  : \
  _type == ESP_MAC_EFUSE_EXT      ? "ESP_MAC_EFUSE_EXT"     : \
                                    "ESP_MAC_UNKNOWN"         \
)

#endif /* __SYS_LUT_H__ */