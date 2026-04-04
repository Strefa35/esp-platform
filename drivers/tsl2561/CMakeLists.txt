message(STATUS "=========| COMPONENTS | TSL2561 |===================================")

#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
)

#####################################
#### INCLUDE_LIST
#####################################
set(INCLUDE_LIST
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

if(CONFIG_TSL2561_ENABLE)
  set(SOURCE_LIST   src/tsl2561.c)
  set(INCLUDE_LIST  include)
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
