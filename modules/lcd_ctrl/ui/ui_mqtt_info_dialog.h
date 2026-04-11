/**
 * @file ui_mqtt_info_dialog.h
 * @brief Modal dialog: MQTT status and basic broker details from the LCD runtime snapshot.
 */

#ifndef __UI_MQTT_INFO_DIALOG_H__
#define __UI_MQTT_INFO_DIALOG_H__

#include <stdbool.h>

/** Strings are null-terminated; filled by the LCD helper before @ref ui_mqtt_info_dialog_show. */
typedef struct {
  bool link_up;
  char broker[64];
  char uid[16];
  char req_topic[48];
} ui_mqtt_info_dialog_data_t;

/**
 * @brief Show the MQTT info dialog.
 *
 * @param data Snapshot of MQTT-related UI strings and link state; must not be NULL.
 */
void ui_mqtt_info_dialog_show(const ui_mqtt_info_dialog_data_t* data);

/** @brief Close the dialog if currently shown. */
void ui_mqtt_info_dialog_close_if_open(void);

#endif /* __UI_MQTT_INFO_DIALOG_H__ */
