/**
 * @file data_types.h
 * @brief Umbrella include: `data.h` plus neutral per-domain layout headers (`data_*.h`).
 *
 * Same convenience as pulling many headers from one place (compare `mgr_reg_list.h`
 * for runtime registration). Use this when you need `data_t` and DTO structs for
 * casting `data_t.data` by `data_type_e`.
 *
 * Each `data_*.h` uses only plain C types (`stdint.h`, `char`, enums defined in that
 * file). No optional ESP-IDF or module headers — so these headers can always be
 * included regardless of Kconfig. Whether Wi-Fi (or other) code is built and linked
 * is decided elsewhere; these files are only shared DTO shapes.
 *
 * Add one line per domain, e.g.:
 *
 *   #include "data_mqtt.h"
 */

#ifndef __DATA_TYPES_H__
#define __DATA_TYPES_H__

#include "data.h"


#endif /* __DATA_TYPES_H__ */
