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

/** Runtime LCD frame context: resolution, partial-render buffer size, and two DMA-capable draw buffers. */
typedef struct {
  int32_t h_res;       /**< Horizontal resolution in pixels. */
  int32_t v_res;       /**< Vertical resolution in pixels. */
  size_t  buffer_size; /**< Bytes per LVGL partial buffer (RGB565). */
  void*   buffer1;     /**< First partial frame buffer (DMA-capable). */
  void*   buffer2;     /**< Second partial frame buffer (DMA-capable); may be NULL for single-buffer mode. */
} lcd_t;


#endif /* __LCD_DEFS_H__ */