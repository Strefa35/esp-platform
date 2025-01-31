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
#include "eth_ctrl.h"


#define MGR_REG_LIST_CNT  ((int)(sizeof(mgr_reg_list)/sizeof(mgr_reg_t)))

static const mgr_reg_t mgr_reg_list[] = {
  /* ETH Controller MUST BE first element in mgr_reg_list */
#ifdef CONFIG_ETH_CTRL
  {
    "/cmd/eth",
    EthCtrl_Init, EthCtrl_Done, EthCtrl_Run, EthCtrl_Send
  },
#endif

#ifdef CONFIG_CLI_CTRL  
  {
    "/cmd/cli",
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_GPIO_CTRL  
  {
    "/cmd/gpio",
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_POWER_CTRL  
  {
    "/cmd/power",
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_TEMPLATE_CTRL  
  {
    "/cmd/template",
    NULL, NULL, NULL, NULL
  },
#endif

#ifdef CONFIG_MQTT_CTRL
  /* MQTT Controller MUST BE last element in mgr_reg_list */
  {
    "/cmd/mqtt",
    NULL, NULL, NULL, NULL
  },
#endif
};


#endif /* __MGR_REG_LIST_H__ */
