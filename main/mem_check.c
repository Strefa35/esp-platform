/**
 * @file mem_check.c
 * @brief Implementation of optional heap snapshot logging (`mem_check` module).
 *
 * @details
 * Built only when `CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE` is set. Samples heap
 * via ESP-IDF heap APIs, formats one log line per call, and optionally runs
 * a FreeRTOS task for periodic logging when `mem_Init(true)` is used.
 */

#include "sdkconfig.h"

#if CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE

#include "mem_check.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/** @brief Single point-in-time heap counters for one log line. */
typedef struct {
  size_t free_heap;      /**< Total free heap (`esp_get_free_heap_size`). */
  size_t free_8bit;      /**< Free bytes in 8-bit capable heap. */
  size_t min_free_8bit;  /**< Minimum free 8-bit heap since boot. */
  size_t largest_8bit;   /**< Largest contiguous free 8-bit block. */
} mem_snapshot_t;

/** @brief Handle for the optional periodic monitor task (NULL until created). */
static TaskHandle_t s_mem_task = NULL;

/** @brief Log tag for `ESP_LOG*` in this module. */
static const char *TAG = "ESP::MEM";

/**
 * @brief Fill @p snapshot_ptr with current heap metrics.
 *
 * @param[in,out] snapshot_ptr Output structure; must not be NULL.
 *
 * @return `ESP_OK` on success, `ESP_ERR_INVALID_ARG` if @p snapshot_ptr is NULL.
 */
static esp_err_t mem_GetSnapshot(mem_snapshot_t *snapshot_ptr) {
  if (snapshot_ptr == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  snapshot_ptr->free_heap = esp_get_free_heap_size();
  snapshot_ptr->free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  snapshot_ptr->min_free_8bit = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  snapshot_ptr->largest_8bit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

  return ESP_OK;
}

/**
 * @brief Emit one `ESP::MEM` info line for @p source / @p label.
 *
 * @param[in] source Calling context label (may be NULL → `"unknown"`).
 * @param[in] label  Stage or checkpoint name (may be NULL → `"snapshot"`).
 */
static void mem_LogHeapLine(const char *source, const char *label) {
  mem_snapshot_t snapshot = {0};
  const char *src = (source != NULL) ? source : "unknown";
  const char *stage = (label != NULL) ? label : "snapshot";

  if (mem_GetSnapshot(&snapshot) != ESP_OK) {
    return;
  }

  ESP_LOGI(TAG,
           "[%s][%s] free_heap=%zu free_8bit=%zu min_free_8bit=%zu largest_8bit=%zu",
           src,
           stage,
           snapshot.free_heap,
           snapshot.free_8bit,
           snapshot.min_free_8bit,
           snapshot.largest_8bit);
}

/**
 * @copydoc mem_LogSnapshot
 */
void mem_LogSnapshot(const char *source, const char *stage, ...) {
  char stage_buf[96] = {};
  const char *label = "snapshot";

  if (stage != NULL) {
    va_list args;
    va_start(args, stage);
    vsnprintf(stage_buf, sizeof(stage_buf), stage, args);
    va_end(args);
    label = stage_buf;
  }

  mem_LogHeapLine(source, label);
}

/**
 * @brief FreeRTOS task: delay, then log a periodic heap line.
 *
 * @param[in] arg Unused (task parameter).
 */
static void mem_MonitorTask(void *arg) {
  (void)arg;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(CONFIG_MAIN_MEMORY_MONITOR_PERIOD_MS));
    mem_LogHeapLine(__func__, "periodic");
  }
}

/**
 * @brief Create the periodic monitor task if not already running.
 *
 * @return `ESP_OK` if the task already exists or was created successfully;
 *         `ESP_FAIL` if `xTaskCreate` failed.
 */
static esp_err_t mem_StartPeriodicMonitor(void) {
  if (s_mem_task != NULL) {
    return ESP_OK;
  }

  BaseType_t task_result = xTaskCreate(
      mem_MonitorTask,
      "mem_mon",
      CONFIG_MAIN_MEMORY_MONITOR_TASK_STACK,
      NULL,
      CONFIG_MAIN_MEMORY_MONITOR_TASK_PRIORITY,
      &s_mem_task);

  return (task_result == pdPASS) ? ESP_OK : ESP_FAIL;
}

/**
 * @copydoc mem_Init
 */
esp_err_t mem_Init(bool start_periodic_monitor) {
  if (!start_periodic_monitor) {
    return ESP_OK;
  }
  return mem_StartPeriodicMonitor();
}

#endif /* CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE */
