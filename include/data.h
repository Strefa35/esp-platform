/**
 * @file data.h
 * @brief Tagged, opaque binary view for bulk payloads (e.g. get_fn callbacks).
 *
 * `data_type_e` selects how to interpret `data`, `count`, and `size`. Concrete
 * struct layouts live in project `include/` (e.g. `data_wifi.h`), pulled in via
 * `data_types.h`. Layout headers use only plain C types so they need no
 * Kconfig guards around `#include`.
 *
 * Lifetime: unless documented otherwise, `data` is valid only for the duration
 * of the synchronous callback or until the producer invalidates the snapshot.
 */

#ifndef __DATA_H__
#define __DATA_H__

#include <stdint.h>

/**
 * Kind of payload carried by `data_t`. Extend with prefixed names per domain
 * (e.g. DATA_WIFI_*, DATA_MQTT_*). Values must stay stable if blobs are stored.
 */
typedef enum {
  DATA_TYPE_NONE = 0,

  /** Placeholder: homogeneous array; layout = TBD in wifi module header (e.g. UI row). */
  DATA_TYPE_WIFI_SCAN_LIST,

  /** Placeholder: single struct or blob for connect attempt outcome; layout TBD. */
  DATA_TYPE_WIFI_CONNECT_STATUS,

  /* Add more kinds as needed; avoid renumbering existing entries. */

  DATA_TYPE_MAX
  
} data_type_e;

typedef struct {
  /** Selects how to interpret `data` / `count` / `size` (per-type contract). */
  data_type_e type;

  /**
   * Number of logical records when the payload is a homogeneous array; use 0
   * when the payload is a single blob or not an array.
   */
  uint32_t count;

  /** Total size of the region pointed to by `data`, in bytes. */
  uint32_t size;

  /** Raw bytes; interpret only according to `type` and owning module docs. */
  const uint8_t *data;
} data_t;

#endif /* __DATA_H__ */
