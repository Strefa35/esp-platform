message(STATUS "=========| MAIN |==================================")

#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
  main.c 
  nvs_ctrl.c
  mgr_ctrl.c
  tools.c
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
  nvs_flash json
)

# Early expansion runs each component CMakeLists in script mode before sdkconfig exists, so CONFIG_*
# is unset and conditional PRIV_REQUIRES below would be skipped. Declare the same optional deps here
# so the component dependency graph and compile include paths stay consistent with the real pass.
if(CMAKE_BUILD_EARLY_EXPANSION)
  list(APPEND PRIV_REQUIRE_LIST
    eth_ctrl
    wifi_ctrl
    esp_wifi
    cfg_ctrl
    sys_ctrl
    cli_ctrl
    gpio_ctrl
    relay_ctrl
    lcd_ctrl
    sensor_ctrl
    template_ctrl
    mqtt_ctrl
  )
endif()

if(CONFIG_ETH_CTRL_ENABLE)
   list(APPEND INCLUDE_LIST  ../modules/eth_ctrl/include)
   list(APPEND PRIV_REQUIRE_LIST eth_ctrl)
endif()

if(CONFIG_WIFI_CTRL_ENABLE)
   list(APPEND INCLUDE_LIST  ../modules/wifi_ctrl/include)
   list(APPEND PRIV_REQUIRE_LIST wifi_ctrl)
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

if(CONFIG_SENSOR_CTRL_ENABLE)
  list(APPEND INCLUDE_LIST  ../modules/sensor_ctrl/include)
  list(APPEND PRIV_REQUIRE_LIST sensor_ctrl)
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
