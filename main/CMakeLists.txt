message(STATUS "=========| MAIN |==================================")

#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
  main.c 
  mgr_ctrl.c
)

#####################################
#### INCLUDE_LIST
#####################################
set(INCLUDE_LIST
  ../include
)

#####################################
#### REQUIRE_LIST
#####################################
set(REQUIRE_LIST
)

#####################################
#### PRIV_REQUIRE_LIST
#####################################
set(PRIV_REQUIRE_LIST 
  fmt json
)

if(CONFIG_ETH_CTRL_ENABLE)
   list(APPEND INCLUDE_LIST  ../modules/eth_ctrl/include)
   list(APPEND PRIV_REQUIRE_LIST eth_ctrl)
endif()

if(CONFIG_CFG_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/cfg_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST cfg_ctrl)
endif()

if(CONFIG_SYS_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/sys_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST sys_ctrl)
endif()

if(CONFIG_CLI_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/cli_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST cli_ctrl)
endif()

if(CONFIG_GPIO_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/gpio_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST gpio_ctrl)
endif()

if(CONFIG_RELAY_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/relay_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST relay_ctrl)
endif()

if(CONFIG_LCD_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/lcd_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST lcd_ctrl)
endif()

#==================================================================
# Example, how to add a new module to the ESP platform
if(CONFIG_TEMPLATE_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/template_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST template_ctrl)
endif()
#==================================================================

if(CONFIG_MQTT_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/mqtt_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST mqtt_ctrl)
endif()

#####################################
#### idf_component_register
#####################################
idf_component_register(
  SRCS ${SOURCE_LIST}
  INCLUDE_DIRS ${INCLUDE_LIST}
  REQUIRES ${REQUIRE_LIST}
  PRIV_REQUIRES ${PRIV_REQUIRE_LIST}
)
