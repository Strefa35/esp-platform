idf_component_register(
  SRCS "eth_ctrl.c"
  INCLUDE_DIRS "include" "../messages/include"
  PRIV_REQUIRES esp_driver_gpio esp_eth esp_netif
)
