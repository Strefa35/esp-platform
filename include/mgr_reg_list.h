/**
 * @file mgr_reg_list.h
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

#ifdef CONFIG_WIFI_CTRL_ENABLE
  #include "wifi_ctrl.h"
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

#ifdef CONFIG_SENSOR_CTRL_ENABLE
  #include "sensor_ctrl.h"
#endif

#ifdef CONFIG_MQTT_CTRL_ENABLE
  #include "mqtt_ctrl.h"
#endif


#define MGR_REG_LIST_CNT  (sizeof(mgr_reg_list)/sizeof(mgr_reg_t))

static mgr_reg_t mgr_reg_list[] = {

  /* ETH Controller MUST BE first element in mgr_reg_list */
#ifdef CONFIG_ETH_CTRL_ENABLE
  {
    .name     = "eth",
    .type     = REG_ETH_CTRL,
    .init_fn  = EthCtrl_Init,
    .done_fn  = EthCtrl_Done,
    .run_fn   = EthCtrl_Run,
    .send_fn  = EthCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_WIFI_CTRL_ENABLE
  {
    .name     = "wifi",
    .type     = REG_WIFI_CTRL,
    .init_fn  = WifiCtrl_Init,
    .done_fn  = WifiCtrl_Done,
    .run_fn   = WifiCtrl_Run,
    .send_fn  = WifiCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_GPIO_CTRL_ENABLE
  {
    .name     = "gpio",
    .type     = REG_GPIO_CTRL,
    .init_fn  = GpioCtrl_Init,
    .done_fn  = GpioCtrl_Done,
    .run_fn   = GpioCtrl_Run,
    .send_fn  = GpioCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_POWER_CTRL_ENABLE
  {
    .name     = "power",
    .type     = REG_POWER_CTRL,
    .init_fn  = PowerCtrl_Init,
    .done_fn  = PowerCtrl_Done,
    .run_fn   = PowerCtrl_Run,
    .send_fn  = PowerCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_RELAY_CTRL_ENABLE
  {
    .name     = "relay",
    .type     = REG_RELAY_CTRL,
    .init_fn  = RelayCtrl_Init,
    .done_fn  = RelayCtrl_Done,
    .run_fn   = RelayCtrl_Run,
    .send_fn  = RelayCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_LCD_CTRL_ENABLE
  {
    .name     = "lcd",
    .type     = REG_LCD_CTRL,
    .init_fn  = LcdCtrl_Init,
    .done_fn  = LcdCtrl_Done,
    .run_fn   = LcdCtrl_Run,
    .send_fn  = LcdCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_CFG_CTRL_ENABLE
  {
    .name     = "cfg",
    .type     = REG_CFG_CTRL,
    .init_fn  = CfgCtrl_Init,
    .done_fn  = CfgCtrl_Done,
    .run_fn   = CfgCtrl_Run,
    .send_fn  = CfgCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_SYS_CTRL_ENABLE
  {
    .name     = "sys",
    .type     = REG_SYS_CTRL,
    .init_fn  = SysCtrl_Init,
    .done_fn  = SysCtrl_Done,
    .run_fn   = SysCtrl_Run,
    .send_fn  = SysCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_CLI_CTRL_ENABLE
  {
    .name     = "cli",
    .type     = REG_CLI_CTRL,
    .init_fn  = CliCtrl_Init,
    .done_fn  = CliCtrl_Done,
    .run_fn   = CliCtrl_Run,
    .send_fn  = CliCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_SENSOR_CTRL_ENABLE
  {
    .name     = "sensor",
    .type     = REG_SENSOR_CTRL,
    .init_fn  = SensorCtrl_Init,
    .done_fn  = SensorCtrl_Done,
    .run_fn   = SensorCtrl_Run,
    .send_fn  = SensorCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_TEMPLATE_CTRL_ENABLE
  {
    .name     = "template",
    .type     = REG_XXX_CTRL,
    .init_fn  = TemplateCtrl_Init,
    .done_fn  = TemplateCtrl_Done,
    .run_fn   = TemplateCtrl_Run,
    .send_fn  = TemplateCtrl_Send,
    .get_fn   = NULL,
  },
#endif

#ifdef CONFIG_MQTT_CTRL_ENABLE
  /* MQTT Controller MUST BE last element in mgr_reg_list */
  {
    .name     = "mqtt",
    .type     = REG_MQTT_CTRL | REG_INT_CTRL,
    .init_fn  = MqttCtrl_Init,
    .done_fn  = MqttCtrl_Done,
    .run_fn   = MqttCtrl_Run,
    .send_fn  = MqttCtrl_Send,
    .get_fn   = NULL,
  },
#endif
};


#endif /* __MGR_REG_LIST_H__ */
