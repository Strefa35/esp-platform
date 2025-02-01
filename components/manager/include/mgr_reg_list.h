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

#ifdef CONFIG_CLI_CTRL_ENABLE

#endif

#ifdef CONFIG_GPIO_CTRL_ENABLE

#endif

#ifdef CONFIG_POWER_CTRL_ENABLE

#endif

#ifdef CONFIG_TEMPLATE_CTRL_ENABLE
  #include "template_ctrl.h"
#endif

#ifdef CONFIG_MQTT_CTRL_ENABLE

#endif


#define MGR_REG_LIST_CNT  (sizeof(mgr_reg_list)/sizeof(mgr_reg_t))

static const mgr_reg_t mgr_reg_list[] = {
  /* ETH Controller MUST BE first element in mgr_reg_list */
#ifdef CONFIG_ETH_CTRL_ENABLE
  {
    "/cmd/eth",
    EthCtrl_Init, EthCtrl_Done, EthCtrl_Run, EthCtrl_Send
  },
#endif

#ifdef CONFIG_CLI_CTRL_ENABLE
  {
    "/cmd/cli",
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_GPIO_CTRL_ENABLE
  {
    "/cmd/gpio",
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_POWER_CTRL_ENABLE
  {
    "/cmd/power",
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_TEMPLATE_CTRL_ENABLE
  {
    "/cmd/template",
    TemplateCtrl_Init, TemplateCtrl_Done, TemplateCtrl_Run, TemplateCtrl_Send
  },
#endif

#ifdef CONFIG_MQTT_CTRL_ENABLE
  /* MQTT Controller MUST BE last element in mgr_reg_list */
  {
    "/cmd/mqtt",
    NULL, NULL, NULL, NULL
  },
#endif
};


#endif /* __MGR_REG_LIST_H__ */
