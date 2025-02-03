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

static mgr_reg_t mgr_reg_list[] = {
  /* ETH Controller MUST BE first element in mgr_reg_list */
#ifdef CONFIG_ETH_CTRL_ENABLE
  {
    MSG_ETH_CTRL,
    EthCtrl_Init, EthCtrl_Done, EthCtrl_Run, EthCtrl_Send
  },
#endif

#ifdef CONFIG_CLI_CTRL_ENABLE
  {
    MSG_CLI_CTRL,
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_GPIO_CTRL_ENABLE
  {
    MSG_CTRL_GPIO,
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_POWER_CTRL_ENABLE
  {
    MSG_POWER_CTRL,
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_TEMPLATE_CTRL_ENABLE
  {
    MSG_TEMPLATE_CTRL,
    TemplateCtrl_Init, TemplateCtrl_Done, TemplateCtrl_Run, TemplateCtrl_Send
  },
#endif

#ifdef CONFIG_MQTT_CTRL_ENABLE
  /* MQTT Controller MUST BE last element in mgr_reg_list */
  {
    MSG_MQTT_CTRL,
    NULL, NULL, NULL, NULL
  },
#endif
};


#endif /* __MGR_REG_LIST_H__ */
