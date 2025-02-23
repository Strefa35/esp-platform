/**
 * @file mgr-reg-list.h
 * @author A.Czerwinski@pistacje.net
 * @brief 
 * @version 0.1
 * @date 2025-01-30
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#ifndef __MGR_REG_LIST_H__
#define __MGR_REG_LIST_H__

#include "mgr_reg.h"

#ifdef CONFIG_ETH_CTRL_ENABLE
  #include "eth_ctrl.h"
#endif

#ifdef CONFIG_GPIO_CTRL_ENABLE
  #include "gpio_ctrl.h"
#endif

#ifdef CONFIG_POWER_CTRL_ENABLE
  #include "power_ctrl.h"
#endif

#ifdef CONFIG_RELAY_CTRL_ENABLE
  #include "relay_ctrl.h"
#endif

#ifdef CONFIG_LCD_CTRL_ENABLE
  #include "lcd_ctrl.h"
#endif

#ifdef CONFIG_CFG_CTRL_ENABLE
  #include "cfg_ctrl.h"
#endif

#ifdef CONFIG_SYS_CTRL_ENABLE
  #include "sys_ctrl.h"
#endif

#ifdef CONFIG_CLI_CTRL_ENABLE
  #include "cli_ctrl.h"
#endif

#ifdef CONFIG_TEMPLATE_CTRL_ENABLE
  #include "template_ctrl.h"
#endif

#ifdef CONFIG_MQTT_CTRL_ENABLE
  #include "mqtt_ctrl.h"
#endif


#define MGR_REG_LIST_CNT  (sizeof(mgr_reg_list)/sizeof(mgr_reg_t))

static mgr_reg_t mgr_reg_list[] = {

  /* ETH Controller MUST BE first element in mgr_reg_list */
#ifdef CONFIG_ETH_CTRL_ENABLE
  {
    "eth", REG_ETH_CTRL,
    EthCtrl_Init, EthCtrl_Done, EthCtrl_Run, EthCtrl_Send
  },
#endif

#ifdef CONFIG_GPIO_CTRL_ENABLE
  {
    "gpio", REG_GPIO_CTRL,
    GpioCtrl_Init, GpioCtrl_Done, GpioCtrl_Run, GpioCtrl_Send
  },
#endif

#ifdef CONFIG_POWER_CTRL_ENABLE
  {
    "power", REG_POWER_CTRL,
    PowerCtrl_Init, PowerCtrl_Done, PowerCtrl_Run, PowerCtrl_Send
  },
#endif

#ifdef CONFIG_RELAY_CTRL_ENABLE
  {
    "relay", REG_RELAY_CTRL,
    RelayCtrl_Init, RelayCtrl_Done, RelayCtrl_Run, RelayCtrl_Send
  },
#endif

#ifdef CONFIG_LCD_CTRL_ENABLE
  {
    "lcd", REG_LCD_CTRL,
    LcdCtrl_Init, LcdCtrl_Done, LcdCtrl_Run, LcdCtrl_Send
  },
#endif

#ifdef CONFIG_CFG_CTRL_ENABLE
  {
    "cfg", REG_CFG_CTRL,
    CfgCtrl_Init, CfgCtrl_Done, CfgCtrl_Run, CfgCtrl_Send
  },
#endif

#ifdef CONFIG_SYS_CTRL_ENABLE
  {
    "sys", REG_SYS_CTRL,
    SysCtrl_Init, SysCtrl_Done, SysCtrl_Run, CfgCtrl_Send
  },
#endif

#ifdef CONFIG_CLI_CTRL_ENABLE
  {
    "cli", REG_CLI_CTRL,
    CliCtrl_Init, CliCtrl_Done, CliCtrl_Run, CliCtrl_Send
  },
#endif

#ifdef CONFIG_TEMPLATE_CTRL_ENABLE
  {
    "template", MSG_XXX_CTRL,
    TemplateCtrl_Init, TemplateCtrl_Done, TemplateCtrl_Run, TemplateCtrl_Send
  },
#endif

#ifdef CONFIG_MQTT_CTRL_ENABLE
  /* MQTT Controller MUST BE last element in mgr_reg_list */
  {
    "mqtt", REG_MQTT_CTRL | REG_INT_CTRL,
    MqttCtrl_Init, MqttCtrl_Done, MqttCtrl_Run, MqttCtrl_Send
  },
#endif
};


#endif /* __MGR_REG_LIST_H__ */
