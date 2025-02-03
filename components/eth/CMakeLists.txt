idf_component_register(
  SRCS "eth_ctrl.c"
  INCLUDE_DIRS "include" "../../include" "../messages/include" "../manager/include"
  PRIV_REQUIRES manager esp_driver_gpio esp_eth esp_netif
)
