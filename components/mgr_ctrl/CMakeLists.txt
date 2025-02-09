idf_component_register(
  SRCS "mgr_ctrl.c"
  INCLUDE_DIRS "include" "../../include" "../eth_ctrl/include" "../template_ctrl/include" "../mqtt_ctrl/include"
  REQUIRES eth_ctrl template_ctrl mqtt_ctrl
  PRIV_REQUIRES fmt json
)
