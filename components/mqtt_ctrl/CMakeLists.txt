idf_component_register(
  SRCS "mqtt_ctrl.c"
  INCLUDE_DIRS "include" "../../include"
  REQUIRES mgr_ctrl
  PRIV_REQUIRES mqtt fmt json
)
