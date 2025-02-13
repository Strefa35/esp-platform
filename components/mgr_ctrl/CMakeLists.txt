idf_component_register(
  SRCS "mgr_ctrl.c"
  INCLUDE_DIRS 
    "include" 
    "../../include" 
    "../eth_ctrl/include" 
    "../cfg_ctrl/include" 
    "../sys_ctrl/include" 
    "../cli_ctrl/include" 
    "../gpio_ctrl/include" 
    "../relay_ctrl/include" 
    "../template_ctrl/include" 
    "../mqtt_ctrl/include"
  REQUIRES 
    eth_ctrl
    cfg_ctrl
    sys_ctrl
    cli_ctrl
    gpio_ctrl
    relay_ctrl
    template_ctrl
    mqtt_ctrl
  PRIV_REQUIRES fmt json
)
