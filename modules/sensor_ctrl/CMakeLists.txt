message(STATUS "=========| MODULES | SENSOR_CTRL |==================================")

option(SENSOR_TSL2561   "Enable Sensor TSL2561"   ON)

message(STATUS, "--------------------------------------------------------------------")
message(STATUS, "          SENSOR TSL2561: ${SENSOR_TSL2561}")
message(STATUS, "--------------------------------------------------------------------")

#####################################
#### SOURCE_LIST
#####################################
set(SOURCE_LIST
  sensor_ctrl.c
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

list(APPEND SOURCE_LIST sensor_tsl2561.c)
list(APPEND INCLUDE_LIST include)
list(APPEND PRIV_REQUIRE_LIST tsl2561)

#####################################
#### idf_component_register
#####################################
idf_component_register(
  SRCS ${SOURCE_LIST}
  INCLUDE_DIRS ${INCLUDE_LIST}
  REQUIRES ${REQUIRE_LIST}
  PRIV_REQUIRES ${PRIV_REQUIRE_LIST}
)
