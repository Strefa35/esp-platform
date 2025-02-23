#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
  "mgr_ctrl.c"
)

#####################################
#### INCLUDE_LIST
#####################################
set(INCLUDE_LIST
  "include"
  "../../include"
)

if(CONFIG_ETH_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../eth_ctrl/include")
endif()

if(CONFIG_CFG_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../cfg_ctrl/include")
endif()

if(CONFIG_SYS_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../sys_ctrl/include")
endif()

if(CONFIG_CLI_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../cli_ctrl/include")
endif()

if(CONFIG_GPIO_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../gpio_ctrl/include")
endif()

if(CONFIG_RELAY_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../relay_ctrl/include")
endif()

if(CONFIG_LCD_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../lcd_ctrl/include")
endif()

if(CONFIG_TEMPLATE_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../template_ctrl/include")
endif()

if(CONFIG_MQTT_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST "../mqtt_ctrl/include")
endif()

#####################################
#### REQUIRE_LIST
#####################################
if(CONFIG_ETH_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST eth_ctrl)
endif()

if(CONFIG_CFG_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST cfg_ctrl)
endif()

if(CONFIG_SYS_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST sys_ctrl)
endif()

if(CONFIG_CLI_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST cli_ctrl)
endif()

if(CONFIG_GPIO_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST gpio_ctrl)
endif()

if(CONFIG_RELAY_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST relay_ctrl)
endif()

if(CONFIG_LCD_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST lcd_ctrl)
endif()

if(CONFIG_TEMPLATE_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST template_ctrl)
endif()

if(CONFIG_MQTT_CTRL_ENABLE)
  list(APPEND REQUIRE_LIST mqtt_ctrl)
endif()

#####################################
#### idf_component_register
#####################################
idf_component_register(
  SRCS ${SOURCE_LIST}
  INCLUDE_DIRS ${INCLUDE_LIST}
  REQUIRES ${REQUIRE_LIST}
  PRIV_REQUIRES fmt json
)
