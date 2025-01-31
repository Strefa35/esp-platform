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

#define MGR_REG_LIST_CNT  (sizeof(mgr_reg_list)/sizeof(mgr_reg_list[0]))

static const mgr_reg_t mgr_reg_list[] = {
  /* ETH Controller MUST BE first element in mgr_reg_list */
  {
    "/cmd/eth",
    EthCtrl_Init, EthCtrl_Done, EthCtrl_Run, EthCtrl_Send
  },
  {
    "/cmd/cli",
    NULL, NULL, NULL, NULL
  },
  {
    "/cmd/gpio",
    NULL, NULL, NULL, NULL
  },
  {
    "/cmd/power",
    NULL, NULL, NULL, NULL
  },
  {
    "/cmd/template",
    NULL, NULL, NULL, NULL
  },
  /* MQTT Controller MUST BE last element in mgr_reg_list */
  {
    "/cmd/mqtt",
    NULL, NULL, NULL, NULL
  },
};

#endif /* __MGR_REG_LIST_H__ */
