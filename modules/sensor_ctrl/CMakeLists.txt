message(STATUS "=========| MODULES | SENSOR_CTRL |==================================")

message(STATUS "--------------------------------------------------------------------")
message(STATUS "      List of sensors:")
message(STATUS "        - TSL2561: ${CONFIG_SENSOR_TSL2561_ENABLE}")
message(STATUS "--------------------------------------------------------------------")

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
  include ${ESP_PLATFORM_DIR}/include
)

#####################################
#### REQUIRE_LIST
#####################################
set(REQUIRE_LIST
  json
)

#####################################
#### PRIV_REQUIRE_LIST
#####################################
set(PRIV_REQUIRE_LIST
  tsl2561
)

if(CONFIG_SENSOR_TSL2561_ENABLE)
  list(APPEND SOURCE_LIST   sensor_tsl2561.c)
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
