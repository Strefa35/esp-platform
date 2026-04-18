/**
 * @file ui_material_icons.h
 * @brief UTF-8 strings for Material Icons (PUA) matching lv_font_material_icons_22.
 *
 * Icon font from https://fonts.google.com/icons (MaterialIcons-Regular subset).
 */

#ifndef UI_MATERIAL_ICONS_H
#define UI_MATERIAL_ICONS_H

#include "lvgl.h"

extern const lv_font_t lv_font_material_icons_22;

/* Material Icons codepoints as UTF-8 (private use area U+E000–U+F8FF).
 * Format: UTF-8 bytes → decoded codepoint → Google Fonts icon name.
 * Regenerate the font with gen_material_icons_font.sh when adding new entries. */
#define MAT_ICON_LAN        "\xEE\xAC\xAF" /* U+EB2F "lan"         */
#define MAT_ICON_WIFI       "\xEE\x98\xBE" /* U+E63E "wifi"        */
#define MAT_ICON_CLOUD      "\xEE\x8A\xBD" /* U+E2BD "cloud"       */
#define MAT_ICON_BLUETOOTH  "\xEE\x86\xA7" /* U+E1A7 "bluetooth"   */
#define MAT_ICON_SETTINGS   "\xEE\xA2\xB8" /* U+E8B8 "settings"    */
#define MAT_ICON_WB_SUNNY   "\xEE\x90\xB0" /* U+E430 "wb_sunny"    */
#define MAT_ICON_MOON       "\xEF\x80\xB6" /* U+F036 "mode_night"  */
#define MAT_ICON_WAVES      "\xEE\x85\xB6" /* U+E176 "waves"       */
#define MAT_ICON_SYNC       "\xEE\x98\xA7" /* U+E627 "sync"        */
#define MAT_ICON_PLAY       "\xEE\x80\xB7" /* U+E037 "play_arrow"  */
#define MAT_ICON_PAUSE      "\xEE\x80\xB4" /* U+E034 "pause"       */

#endif /* UI_MATERIAL_ICONS_H */
