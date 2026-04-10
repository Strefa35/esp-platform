/**
 * @file mem_check.h
 * @brief Optional heap snapshot logging API (Kconfig-controlled).
 *
 * @details
 * When `CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE` is set, this header declares
 * `mem_Init()` and `mem_LogSnapshot()` and defines `MEM_CHECK()`.
 * When it is unset, `MEM_CHECK()` expands to an empty `do { } while (0)` so call
 * sites compile out without linking the `mem_check` implementation, while staying
 * a single statement (safe after `if` / `else`, loops, etc.).
 */

#ifndef __MEM_CHECK_H__
#define __MEM_CHECK_H__

#include "sdkconfig.h"

#if CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE

#include <stdbool.h>

#include "esp_err.h"

/**
 * @brief Initialize the memory diagnostics module (call once from `app_main`).
 *
 * @param[in] start_periodic_monitor If true, starts the background task that
 *            logs heap lines periodically (requires `CONFIG_MAIN_MEMORY_PERIODIC_MONITOR_ENABLE`
 *            so `CONFIG_MAIN_MEMORY_MONITOR_*` are defined). If false, returns `ESP_OK`
 *            without creating that task.
 *
 * @return `ESP_OK` on success or when the periodic task is not requested;
 *         `ESP_FAIL` if the monitor task could not be created;
 *         `ESP_ERR_NOT_SUPPORTED` if @p start_periodic_monitor is true but the periodic
 *         monitor was not enabled in Kconfig (implementation omitted).
 */
esp_err_t mem_Init(bool start_periodic_monitor);

/**
 * @brief Log one heap snapshot line with source and stage labels.
 *
 * Output uses tag `ESP::MEM` and includes free heap, 8-bit free space,
 * minimum free 8-bit, and largest free 8-bit block.
 *
 * @param[in] source Caller label, usually `__func__`.
 * @param[in] stage  `printf`-style format string for the checkpoint name.
 * @param[in] ...    Optional arguments for @p stage.
 */
void mem_LogSnapshot(const char *source, const char *stage, ...);

/**
 * @brief Single-statement wrapper for @p stmt when snapshot logging is enabled.
 *
 * Uses `do { ... } while (0)` so the expansion is one statement (e.g. safe for
 * `if (c) MEM_CHECK(...); else ...`). Place a semicolon after the invocation, e.g.:
 * `MEM_CHECK(mem_LogSnapshot(__func__, "done"));`
 *
 * @param[in] stmt A complete C statement (typically a `mem_*` call).
 */
#define MEM_CHECK(stmt) \
    do {                  \
        stmt;             \
    } while (0)

#else /* !CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE */

/**
 * @brief No-op single statement: snapshot API is disabled; @p stmt is not expanded
 *        or compiled (argument is not substituted).
 * @param[in] stmt Ignored.
 */
#define MEM_CHECK(stmt) \
    do {                  \
    } while (0)

#endif /* CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE */

#endif /* __MEM_CHECK_H__ */
