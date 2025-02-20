idf_component_register(
  SRCS "relay_ctrl.c"
  INCLUDE_DIRS "include" "../../include"
  PRIV_REQUIRES esp_driver_gpio fmt json
)
