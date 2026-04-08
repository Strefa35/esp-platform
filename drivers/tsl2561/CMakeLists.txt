message(STATUS "=========| COMPONENTS | TSL2561 |===================================")

#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
)

#####################################
#### INCLUDE_LIST
#####################################
# Headers are always visible so dependents (e.g. sensor_ctrl) can include tsl2561.h
# whenever the sensor or driver is enabled; object code is built only when needed.
set(INCLUDE_LIST
  include
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
  esp_driver_gpio
  esp_driver_i2c
)

if(CONFIG_TSL2561_ENABLE OR CONFIG_SENSOR_TSL2561_ENABLE)
  set(SOURCE_LIST src/tsl2561.c)
endif()

#####################################
#### idf_component_register
#####################################
idf_component_register(
  SRCS          ${SOURCE_LIST}
  INCLUDE_DIRS  ${INCLUDE_LIST}
  REQUIRES      ${REQUIRE_LIST}
  PRIV_REQUIRES ${PRIV_REQUIRE_LIST}
)
