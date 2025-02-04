idf_component_register(
  SRCS "mqtt_ctrl.c"
  INCLUDE_DIRS "include" "../../include" "../msg_ctrl/include"
  PRIV_REQUIRES mqtt mgr_ctrl
)
