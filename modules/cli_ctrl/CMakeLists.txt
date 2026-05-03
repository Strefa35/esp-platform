message(STATUS "=========| MODULES | CLI_CTRL |==================================")

#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
  cli_ctrl.c
)

if(CONFIG_WIFI_CTRL_ENABLE)
  list(APPEND SOURCE_LIST cli_wifi.c)
endif()

if(CONFIG_LCD_CTRL_ENABLE)
  list(APPEND SOURCE_LIST cli_lcd.c)
endif()

#####################################
#### INCLUDE_LIST
#####################################
set(INCLUDE_LIST
  include ../../include
)

#####################################
#### PRIV_REQUIRE_LIST
#####################################
set(PRIV_REQUIRE_LIST
  console
)

#####################################
#### idf_component_register
#####################################
idf_component_register(
  SRCS ${SOURCE_LIST}
  INCLUDE_DIRS ${INCLUDE_LIST}
  PRIV_REQUIRES ${PRIV_REQUIRE_LIST}
)
