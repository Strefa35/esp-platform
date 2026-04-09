# Memory usage measurement (ESP-IDF)

This document describes how to measure:

- flash/image usage after build
- RAM usage at runtime (early start and after system startup)

The firmware also provides a small **`mem` module** (`include/mem.h`, `main/mem.c`) for consistent heap snapshots in logs. Public functions and macros are described in **Doxygen** comments in those files (suitable for `doxygen` or IDF doc generation).

---

## 1) Flash/image usage after build

Run from project root:

```bash
idf.py build
idf.py size
```

Useful detailed views:

```bash
idf.py size-components
idf.py size-files
```

What you get:

- `idf.py size` — summary of code/data footprint in flash and RAM-related sections
- `idf.py size-components` — contribution by component
- `idf.py size-files` — contribution by object/source file

You can also inspect the linker map file:

- `build/<project_name>.map`

This is helpful when a specific symbol or section unexpectedly grows.

### Readable per-archive / per-file tables

The terminal tables from `idf.py size-components` and `idf.py size-files` can wrap badly in narrow consoles. For machine-readable output, use the MAP file with ESP-IDF’s size tool:

```bash
python -m esp_idf_size --format json --archives build/esp-platform.map -o build/size-archives.json
python -m esp_idf_size --format json --files   build/esp-platform.map -o build/size-files.json
```

(Use your actual `build/<app>.map` name if it differs.)

---

## 1.1) Example footprint analysis (this repository)

The figures below are a **representative snapshot** from one `idf.py build` on this project (ESP-IDF 5.5, target as configured in the workspace). **Re-run `idf.py size` after your own build** before making release decisions; numbers move with `sdkconfig`, LVGL fonts, and enabled modules.

### Image and section summary (`idf.py size`)

| Metric | Value (approx.) | Notes |
|--------|-----------------|--------|
| Application `.bin` size | ~0x185d50 (~1.59 MiB) | Padded `.bin` may differ slightly from ELF total |
| Free space in smallest app partition | ~13% (~246 KiB) | From partition check at end of build |
| Flash code (`.text` in flash) | ~1.09 MiB | |
| Flash read-only data (`.rodata` + app descriptor) | ~329 KiB | |
| IRAM used | ~110 KiB / 128 KiB (~86%) | Tight headroom |
| DRAM used (`.data` + `.bss` in internal RAM) | ~104 KiB / ~177 KiB (~59%) | `.bss` dominates (~86 KiB) |
| Total image size reported by size tool | ~1.52 MiB | May differ from TRM “total internal” definitions |

### Largest static contributors (archives)

Rough ordering by **flash** (`flash_total` in `esp_idf_size` JSON):

| Archive | Flash (approx.) | RAM (`ram_st_total`, approx.) |
|---------|-----------------|-------------------------------|
| `liblvgl__lvgl.a` | ~377 KiB | ~66 KiB |
| `libnet80211.a` | ~162 KiB | ~15 KiB |
| `libesp_app_format.a` | ~144 KiB | negligible | Large `.rodata` in `esp_app_desc.o` (metadata, not “logic”) |
| `liblwip.a` | ~103 KiB | ~4 KiB |
| `libmbedcrypto.a` | ~90 KiB | small |
| `libc.a` | ~84 KiB | small |
| `libpp.a` | ~72 KiB | ~26 KiB |
| `libwpa_supplicant.a` | ~64 KiB | small |
| `libphy.a` | ~44 KiB | ~10 KiB |

**LVGL** is the single largest block: it includes **fonts and themes in flash** and a **fixed LVGL heap in `.bss`** (see `CONFIG_LV_MEM_SIZE_KILOBYTES` in `sdkconfig`; the builtin allocator object file typically accounts for **64 KiB** BSS when set to 64).

**Wi-Fi / network** (`net80211`, `pp`, `lwip`, `wpa_supplicant`, `phy`, crypto) accounts for most of the remainder after LVGL.

### In-tree modules (object-level highlights)

Within **this repo’s** linked objects (not IDF libs), typical large contributors include:

| Area | Flash (approx.) | DRAM (approx.) | Typical object |
|------|-----------------|----------------|----------------|
| MQTT control | ~12 KiB | ~0.4 KiB | `libmqtt_ctrl.a:mqtt_ctrl.c.obj` |
| Manager | ~8 KiB | ~0.8 KiB | `libmain.a:mgr_ctrl.c.obj` |
| System control | ~7 KiB | ~0.1 KiB | `libsys_ctrl.a:sys_ctrl.c.obj` |
| Wi-Fi control | ~3 KiB | ~2 KiB | `libwifi_ctrl.a:wifi_ctrl.c.obj` (relatively more RAM) |
| LCD / UI | several KiB each | varies | `lcd_ctrl.c`, `ui_main_screen.c`, `lcd_helper.c`, screensaver, UI fonts |

Single-file **flash** standouts in LVGL builds often include **`lv_font_montserrat_46`** (~90+ KiB `.rodata`) and other enabled Montserrat sizes; trim font enable list in menuconfig to shrink flash.

### Configuration knobs in this project worth reviewing

- **LVGL:** `CONFIG_LV_MEM_SIZE_KILOBYTES`, `CONFIG_LV_DRAW_LAYER_SIMPLE_BUF_SIZE`, enabled fonts/widgets, color depth.
- **Tasks:** e.g. `CONFIG_ESP_MAIN_TASK_STACK_SIZE`, `LCD_UI_TASK_STACK_SIZE` (16 KiB in `lcd_helper.c`), `CLI_CTRL_REPL_STACK_SIZE` (8 KiB class), other module `*_TASK_STACK_SIZE` defines.
- **Wi-Fi buffers:** `CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM`, `CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM`, `CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM`, `CONFIG_ESP_WIFI_MGMT_SBUF_NUM` (see comments in `sdkconfig.defaults` re. PHY calibration and heap).

### Takeaway

- **Flash and static RAM:** dominated by **LVGL** (fonts + builtin pool + draw buffers) and the **IDF Wi-Fi / TCP/IP stack**.
- **Application modules:** **MQTT**, **mgr**, **sys**, **wifi**, and **LCD/UI** layers are the main in-tree costs; they are smaller than LVGL+WiFi but are where **project-specific** tuning has the most effect.

---

## 2) Runtime heap snapshots (`mem` module)

### menuconfig

Open **ESP32 - Platform → MAIN**:

| Option | Meaning |
|--------|---------|
| **Enable memory snapshot logs** (`CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE`) | When enabled, the `mem` API and `MEM_CHECK(...)` call sites compile in; heap lines go to the log (tag `ESP::MEM`). When off, those call sites compile to empty statements—no stub functions, no linker references. Default: **off**. |
| **Enable periodic memory monitor task** (`CONFIG_MAIN_MEMORY_PERIODIC_MONITOR_ENABLE`) | Depends on snapshot enable. Exposes period/stack/priority symbols used when creating the monitor task. **`app_main`** passes this value into **`mem_Init(bool)`** so the default follows menuconfig; you may pass **`false`** at runtime to skip starting the task even when this option is on. Default: **off**. |
| **Memory monitor period [ms]** (`CONFIG_MAIN_MEMORY_MONITOR_PERIOD_MS`) | Interval between periodic log lines (1000–600000, default 10000). The task **waits** this long **before** the first `periodic` line, then after each line. |
| **Memory monitor task stack [bytes]** (`CONFIG_MAIN_MEMORY_MONITOR_TASK_STACK`) | Stack size for the monitor task (2048–8192, default 3072). |
| **Memory monitor task priority** (`CONFIG_MAIN_MEMORY_MONITOR_TASK_PRIORITY`) | FreeRTOS priority (1–10, default 2). |

After changing options, rebuild (`idf.py build`).

### Headers, macros, and conditional API (`include/mem.h`)

`mem.h` includes `sdkconfig.h` and exposes symbols **only when snapshot logging is enabled**:

- When **`CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE`** is on: **`mem_Init(bool)`**, **`mem_LogSnapshot()`**, and **`MEM_CHECK(stmt)`** (expands to `stmt`). Implementation details and **`static`** helpers live in **`main/mem.c`**.
- When it is off: **`MEM_CHECK(stmt)`** is still defined but expands to nothing—wrap snapshot / **`mem_Init`** call sites so they compile out (no `mem_*` declarations in that build).

### Implementation notes (`main/mem.c`)

- The translation unit is built only when **`CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE`** is set.
- **Internal** pieces (not in the header): **`mem_snapshot_t`**, **`mem_GetSnapshot`**, **`mem_LogHeapLine`**, **`mem_MonitorTask`**, **`mem_StartPeriodicMonitor`**. They feed **`mem_LogSnapshot`** and the optional periodic task.
- **FreeRTOS** headers are included whenever snapshots are enabled; the periodic task is created only if **`mem_Init(true)`** is used (e.g. **`mem_Init(CONFIG_MAIN_MEMORY_PERIODIC_MONITOR_ENABLE)`** in **`main/main.c`**). Task parameters (**`CONFIG_MAIN_MEMORY_MONITOR_*`**) are the menuconfig values that appear when **Enable periodic memory monitor task** is on—do not call **`mem_Init(true)`** in a configuration where those macros are missing.
- The monitor loop calls **`vTaskDelay`** with the configured period **before** each **`[mem_MonitorTask][periodic]`** line (including the first iteration).

#### Public functions (when compiled in)

```c
esp_err_t mem_Init(bool start_periodic_monitor);

void mem_LogSnapshot(const char *source, const char *stage, ...);
```

- **`mem_Init`** — call **once** from **`app_main`** (after optional early **`mem_LogSnapshot`**). If **`start_periodic_monitor`** is **`false`**, returns **`ESP_OK`** and does **not** create the periodic task. If **`true`**, calls **`static mem_StartPeriodicMonitor()`** in **`mem.c`**: returns **`ESP_OK`** if the task already exists or **`xTaskCreate`** succeeds, else **`ESP_FAIL`**. Typical default: **`mem_Init(CONFIG_MAIN_MEMORY_PERIODIC_MONITOR_ENABLE)`** in **`main/main.c`** so menuconfig controls whether **`true`** is passed. Use **`mem_Init(false)`** to keep checkpoint logging but skip the background task for a given boot.
- **`mem_LogSnapshot`** — reads the heap (internally), then logs one line. `source` is usually `__func__`. `stage` is a **printf-style format string**; optional arguments follow (e.g. module name).

The log fields **`free_heap`**, **`free_8bit`**, **`min_free_8bit`**, and **`largest_8bit`** come from the same internal snapshot used for **`mem_LogSnapshot`**; there is no exported **`mem_GetSnapshot`** or **`mem_snapshot_t`**—call **`esp_get_free_heap_size()`** / **`heap_caps_*`** in your own code if you need numeric access outside this module.

#### Call-site pattern (recommended)

Use **`MEM_CHECK`** for any single statement that should exist only when snapshot logging is enabled (semicolon **after** the closing parenthesis of the macro):

```c
MEM_CHECK(mem_LogSnapshot(__func__, "after_nvs_init"));
MEM_CHECK(mem_LogSnapshot(__func__, "mgr_run_module_done:%s", module_name));
```

Call **`mem_Init`** from **`app_main`** inside a compound statement wrapped with **`MEM_CHECK`** so the whole block is omitted when snapshots are disabled (see **`main/main.c`**). The block assumes an outer **`esp_err_t result`**:

```c
MEM_CHECK({
  result = mem_Init(CONFIG_MAIN_MEMORY_PERIODIC_MONITOR_ENABLE);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - mem_Init() failed", __func__);
  }
});
```

Log line shape (when snapshots are enabled):

```text
I (t) ESP::MEM: [source][stage] free_heap=... free_8bit=... min_free_8bit=... largest_8bit=...
```

Examples (as in the tree):

```c
MEM_CHECK(mem_LogSnapshot(__func__, "after_nvs_init"));
MEM_CHECK(mem_LogSnapshot(__func__, "mgr_run_module_done:%s", module_name));
```

### Where it is used in this project

Checkpoints are wired in:

- **`main/main.c`** — `MEM_CHECK(mem_LogSnapshot(...))` at early boot and after `tools_Init`, `NVS_Init`, `MGR_Init`, `MGR_Run`; **`MEM_CHECK({ result = mem_Init(CONFIG_MAIN_MEMORY_PERIODIC_MONITOR_ENABLE); ... })`** once near the start (periodic task only if Kconfig allows it **and** the argument is true).
- **`main/mgr_ctrl.c`** — same **`MEM_CHECK(mem_LogSnapshot(...))`** pattern for manager lifecycle and per-module steps (stage strings include the registered module name where relevant).

**Convention:** only **`main.c`** should call **`mem_Init`**. **`main.c`** and **`mgr_ctrl.c`** use **`MEM_CHECK` / `mem_LogSnapshot`** at checkpoints. Implementation stays in **`main/mem.c`**; do not add `#include "mem.h"` in other components unless you deliberately extend this policy.

### Interpreting values

- **`free_heap` / `free_8bit`** — currently available dynamic RAM (diagnostic snapshot).
- **`min_free_8bit`** — minimum free 8-bit-capable heap seen so far (high-water mark of pressure).
- **`largest_8bit`** — largest contiguous free block (fragmentation indicator).

If `largest_8bit` drops quickly while total free heap still looks acceptable, fragmentation may be an issue.

**Threading:** heap snapshot reads are intended for diagnostics; values are inherently momentary. You do not need a dedicated semaphore around **`mem_LogSnapshot`** for normal use. **`mem_Init`** is idempotent for the periodic task (second call is a no-op while the task already exists); call it only from **`app_main`** unless you add explicit guards for other designs.

---

## 3) Optional ad-hoc monitoring (custom code)

If you add your own loop (outside the built-in monitor task), enable **memory snapshot logs** in menuconfig and use **`MEM_CHECK`** so the code drops out when the option is off:

```c
for (;;) {
    MEM_CHECK(mem_LogSnapshot(__func__, "runtime_tick"));
    vTaskDelay(pdMS_TO_TICKS(10000));
}
```

To read heap metrics in application code without going through **`mem_LogSnapshot`**, use ESP-IDF APIs directly (e.g. **`esp_get_free_heap_size()`**, **`heap_caps_get_free_size(MALLOC_CAP_8BIT)`**, **`heap_caps_get_minimum_free_size`**, **`heap_caps_get_largest_free_block`**)—that mirrors what **`main/mem.c`** does internally.

Alternatively, wrap custom code in `#if CONFIG_MAIN_MEMORY_SNAPSHOT_ENABLE` if you prefer not to use **`MEM_CHECK`** for multi-line logic.

---

## 4) Practical measurement checklist

1. Build and save size reports:
   - `idf.py size`
   - `idf.py size-components`
2. Enable **MAIN → Enable memory snapshot logs** (and optional periodic monitor), rebuild, flash.
3. Boot and capture log lines from **`ESP::MEM`**:
   - `app_main_begin`, `after_*` checkpoints
   - manager checkpoints and `mgr_*_module_done:<name>` stages
   - optional **`[mem_MonitorTask][periodic]`** lines when **`mem_Init(true)`** is used
4. Run a normal workload for 10–30 minutes and watch trends (especially `min_free_8bit` and periodic lines if enabled).
5. Compare against a baseline on the **same target** and **similar sdkconfig**.

---

## 5) Notes

- Build-time size reports and runtime heap metrics answer different questions; both are useful.
- Always compare runs with the same chip target and comparable `sdkconfig`; otherwise numbers are not directly comparable.
- Logging to a file on the device (SPIFFS/LittleFS/FAT) is not implemented in `mem`; capture serial output on the host if you need a log file without extra flash wear.
