idf_component_register(
  SRCS "eth_ctrl.c"
  INCLUDE_DIRS "include" "../../include" "../msg_ctrl/include" "../mgr_ctrl/include"
  PRIV_REQUIRES esp_driver_gpio esp_eth esp_netif mgr_ctrl
)
