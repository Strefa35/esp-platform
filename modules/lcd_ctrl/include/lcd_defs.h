/**
 * @file lcd_defs.h
 * @author A.Czerwinski@pistacje.net
 * @brief LCD definitions
 * @version 0.1
 * @date 2025-02-24
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __LCD_DEFS_H__
#define __LCD_DEFS_H__

#include <stdio.h>
#include <stdbool.h>

typedef struct {
  int32_t h_res;
  int32_t v_res;
  size_t  buffer_size;
  void*   buffer1;
  void*   buffer2;
} lcd_t;


#endif /* __LCD_DEFS_H__ */