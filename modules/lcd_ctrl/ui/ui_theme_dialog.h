/**
 * @file ui_theme_dialog.h
 * @brief Modal dialog: background gradient theme selector.
 */

#ifndef __UI_THEME_DIALOG_H__
#define __UI_THEME_DIALOG_H__

/**
 * @brief Open the theme selector dialog on the LVGL top layer.
 *
 * Each row shows a gradient swatch, theme name, and a checkmark on the active
 * theme.  Selecting a row applies the theme immediately via
 * @ref ui_main_screen_set_theme.  Safe to call when already open (re-opens).
 */
void ui_theme_dialog_show(void);

/** @brief Close the dialog if currently shown; no-op otherwise. */
void ui_theme_dialog_close_if_open(void);

#endif /* __UI_THEME_DIALOG_H__ */
