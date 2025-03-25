message(STATUS "=========| MODULES | SENSORS_CTRL |==================================")

#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
  sensors_ctrl.c
)

#####################################
#### INCLUDE_LIST
#####################################
set(INCLUDE_LIST
  include ../../include
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
)

if(CONFIG_SENSOR_TSL2561_ENABLE)
  list(APPEND SOURCE_LIST sensor_tsl2561.c)
  list(APPEND INCLUDE_LIST include)
  list(APPEND PRIV_REQUIRE_LIST tsl2561)
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
