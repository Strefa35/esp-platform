/**
 * @file tsl2561_reg.h
 * @author A.Czerwinski@pistacje.net
 * @brief List of TSL2561 registers
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __TSL2561_REG_H__
#define __TSL2561_REG_H__


/* ======================== */
/*     Register Address     */
/* ======================== */

#define TSL2561_REG_COMMAND             0x80

/* 0x00 - Control of basic functions  */
#define	TSL2561_REG_CONTROL             0x00

/* 0x01 - Integration time/gain control */
#define	TSL2561_REG_TIMING              0x01

/* 0x02 - Low byte of low interrupt threshold   */ 
/* 0x03 - High byte of low interrupt threshold  */
#define	TSL2561_REG_THRESH_L            0x02

/* 0x04 - Low byte of high interrupt threshold  */
/* 0x05 - High byte of high interrupt threshold */
#define	TSL2561_REG_THRESH_H            0x04

/* Interrupt */
#define	TSL2561_REG_INTCTL              0x06

/* Part number/ Rev ID - */
#define	TSL2561_REG_ID                  0x0A

/* 0x0C - Low byte of ADC channel 0   */
/* 0x0D - High byte of ADC channel 0  */
#define	TSL2561_REG_DATA_0              0x0C

/* 0x0E - Low byte of ADC channel 1   */
/* 0x0F - High byte of ADC channel 1  */
#define	TSL2561_REG_DATA_1              0x0E

/* ======================== */
/*     Command Register     */
/* ======================== */
#define TSL2561_CMD_INTR_CLEAR          0x40
#define TSL2561_CMD_WORD_PROTOCOL       0x20
#define TSL2561_CMD_BLOCK_PROTOCOL      0x10

/* ======================== */
/*     Control Register     */
/* ======================== */
#define TSL2561_CTRL_POWER_ON           0x03
#define TSL2561_CTRL_POWER_OFF          0x00

/* ======================== */
/*     Timing Register      */
/* ======================== */
#define TSL2561_TIMING_GAIN             0x10
#define TSL2561_TIMING_MANUAL           0x80
#define TSL2561_TIMING_INTEGRATE        0x03

/* ======================== */
/*     Interrupt Register   */
/* ======================== */
#define TSL2561_INTR_DISABLE            0x00
#define TSL2561_INTR_LEVEL              0x10
#define TSL2561_INTR_SMB_ALERT          0x20
#define TSL2561_INTR_TEST_MODE          0x30

#define TSL2561_INTR_EVERY_ADC          0x00
#define TSL2561_INTR_OUTSIDE_THRESHOLD  0x01

#endif /* __TSL2561_REG_H__ */
