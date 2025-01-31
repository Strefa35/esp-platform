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

#define MGR_REG_LIST_CNT  (sizeof(mgr_reg_list)/sizeof(mgr_reg_list[0]))

static const mgr_reg_t mgr_reg_list[] = {
  {
    "/cmd/eth",
    NULL, NULL, NULL, NULL
  }
};

#endif /* __MGR_REG_LIST_H__ */
