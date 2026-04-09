# Heap log parser (`scripts/parse_mem_log.py`)

This script reads a serial or **idf_monitor** log that contains `ESP::MEM` lines emitted by the **`mem`** module (`mem_LogSnapshot` checkpoints and, when enabled, periodic lines with stage **`periodic`** from `mem_MonitorTask`). It prints Markdown tables with **approximate per-module heap deltas** for the manager init and run phases.

## Requirements

- **Python 3** (standard library only; no `pip` packages)
- Firmware built with **memory snapshot logging** enabled so the log includes:
  - `MGR_Init` stages: `mgr_init_begin`, `mgr_init_module_done: <name>`, `mgr_init_done`
  - `MGR_Run` stages: `mgr_run_begin`, `mgr_run_module_done: <name>`

Enable these in menuconfig if needed (see [memory.md](memory.md) for the monitoring options).

## Usage

From the project root:

```bash
python3 scripts/parse_mem_log.py path/to/monitor.log
```

Read from stdin (e.g. pipe monitor output):

```bash
idf.py -p PORT monitor 2>&1 | tee monitor.log | python3 scripts/parse_mem_log.py -
```

Or analyze a capture after the fact:

```bash
python3 scripts/parse_mem_log.py monitor.log > heap-report.md
```

Use `-` or omit the file argument when piping; the default input is **stdin** when you pass `-` explicitly (see `python3 scripts/parse_mem_log.py --help`).

## What the script extracts

1. **First init block** — from the first `mgr_init_begin` through the matching `mgr_init_done`.
2. **First run segment** — from the first `mgr_run_begin` after that block, through consecutive `mgr_run_module_done` lines until another `mgr_run_begin` or the end of the log.

ANSI color codes are stripped so logs from **idf_monitor** are accepted as-is.

## How to read the tables

- **`free_heap after init` / `free_heap after run`** — value reported by `esp_get_free_heap_size()` at that checkpoint (bytes).
- **Δ from prev (B)** — difference between the previous row’s `free_heap` and the current one:  
  `previous_free_heap - current_free_heap`.  
  This is treated as **net heap consumed** between two consecutive snapshots for that module step.
- **Σ rows** — sum of step deltas should match the overall drop from the baseline line to the last line in that section (within the same block).
- **Italic footnotes** after each table — when the log line includes `free_8bit`, `min_free_8bit`, and `largest_8bit` (as printed by current firmware), the script repeats those fields at **`mgr_init_done`** and after the **last** `mgr_run_module_done` in the analyzed segment. Deltas in the tables still use **`free_heap` only**; the extra fields help spot fragmentation / high-water marks (see [memory.md](memory.md)). Lines with only `free_heap=` are still accepted.

## Limitations

- Deltas attribute changes to **the module at the snapshot**; other tasks, Wi-Fi/LwIP, or drivers may allocate or free memory between snapshots, so numbers are **indicative**, not a precise per-module allocator account.
- Only the **first** complete init block and the **first** run segment (as defined above) are analyzed. Logs with multiple reboots or multiple run cycles need manual splitting or script changes if you need later cycles.
- Corrupted log lines (e.g. terminal control sequences breaking a line) may omit a snapshot; if a line still contains `ESP::MEM` and `free_heap=<digits>`, it is parsed.

## Related files

- `scripts/parse_mem_log.py` — implementation
- `include/mem.h` / `main/mem.c` — heap logging API and implementation (see Doxygen in sources)
- `main/main.c` — `mem_Init`, `MEM_CHECK(mem_LogSnapshot(...))` boot checkpoints
- `main/mgr_ctrl.c` — `mem_LogSnapshot` call sites for init/run
- [memory.md](memory.md) — firmware memory monitoring configuration
