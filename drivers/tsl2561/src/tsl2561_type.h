/**
 * @file tsl2561_type.h
 * @author A.Czerwinski@pistacje.net
 * @brief List of TSL2561 types
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __TSL2561_TYPE_H__
#define __TSL2561_TYPE_H__


typedef enum {
  PART_TSL2560CS        = 0x00,
  PART_TSL2561CS        = 0x10,
  PART_TSL2560T_FN_CL   = 0x40,
  PART_TSL2561T_FN_CL   = 0x50,

  PART_TSL2561_UNKNOWN  = 0xFF
} tsl2561_partno_e;

typedef struct tsl2561_s {
  i2c_master_bus_handle_t bus;
  i2c_master_dev_handle_t device;

  tsl2561_gain_e          gain;
  tsl2561_integ_e         integ;
  tsl2561_partno_e        partno;
  uint8_t                 revno;

  uint16_t                broadband;
  uint16_t                infrared;
  uint32_t                lux;
  float                   ratio;

  uint16_t                ms;

  tsl2561_isr_f           isr_notify;
  tsl2561_threshold_t     threshold;
} tsl2561_s;

#endif /* __TSL2561_TYPE_H__ */