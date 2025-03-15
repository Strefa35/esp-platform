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

#####################################
#### idf_component_register
#####################################
idf_component_register(
  SRCS ${SOURCE_LIST}
  INCLUDE_DIRS ${INCLUDE_LIST}
  REQUIRES ${REQUIRE_LIST}
  PRIV_REQUIRES ${PRIV_REQUIRE_LIST}
)
