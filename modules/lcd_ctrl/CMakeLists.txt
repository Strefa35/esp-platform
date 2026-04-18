message(STATUS "=========| MODULES | LCD_CTRL |==================================")

#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
  lcd_ctrl.c
  lcd_helper.c
  lcd_hw.c
  ili9341v.c
  ns2009.c
  fonts/lv_font_material_icons_22.c
  ui/ui_main_screen.c
  ui/ui_eth_info_dialog.c
  ui/ui_wifi_info_dialog.c
  ui/ui_mqtt_info_dialog.c
  ui/ui_theme_dialog.c
  ui/ui_screensaver.c
)

#####################################
#### INCLUDE_LIST
#####################################
set(INCLUDE_LIST
  include ../../include ui
)

#####################################
#### REQUIRE_LIST
#####################################
set(REQUIRE_LIST
  driver
  esp_lcd
  esp_netif
  esp_timer
  lvgl
)

#####################################
#### PRIV_REQUIRE_LIST
#####################################
set(PRIV_REQUIRE_LIST 
  json
)

#####################################
#### idf_component_register
#####################################
idf_component_register(
  SRCS ${SOURCE_LIST}
  INCLUDE_DIRS ${INCLUDE_LIST}
  REQUIRES ${REQUIRE_LIST}
  PRIV_REQUIRES ${PRIV_REQUIRE_LIST}
)
