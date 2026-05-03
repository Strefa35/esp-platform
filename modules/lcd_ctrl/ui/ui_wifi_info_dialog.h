/**
 * @file ui_wifi_info_dialog.h
 * @brief Modal dialog: Wi-Fi STA link and IPv4 details from the LCD runtime snapshot.
 */

#ifndef __UI_WIFI_INFO_DIALOG_H__
#define __UI_WIFI_INFO_DIALOG_H__

#include <stdbool.h>

/** Strings are null-terminated; filled by the LCD helper before @ref ui_wifi_info_dialog_show. */
typedef struct {
  char ip[24];
  char netmask[24];
  char gateway[24];
  char mac[24];
  bool link_up;
} ui_wifi_info_dialog_data_t;

/**
 * @brief Show a centered modal over @c lv_layer_top(), or refresh if one is already open.
 *
 * Must be called from the LVGL thread (e.g. input or timer callback).
 *
 * @param data Snapshot of Wi-Fi-related UI strings and link state; must not be NULL.
 */
void ui_wifi_info_dialog_show(const ui_wifi_info_dialog_data_t* data);

/** Remove the dialog from @c lv_layer_top() if it is open (e.g. before changing root screen). */
void ui_wifi_info_dialog_close_if_open(void);

#endif /* __UI_WIFI_INFO_DIALOG_H__ */
