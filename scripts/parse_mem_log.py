#!/usr/bin/env python3
"""
Parse idf_monitor / serial logs for ESP::MEM heap snapshots and print
per-module delta tables for MGR_Init and MGR_Run (same methodology as manual
analysis: delta = previous free_heap - current free_heap at each step).

Log lines match firmware output from main/mem_check.c (heap fields printed as decimal
integers via %zu for size_t), e.g.:
  I (t) ESP::MEM: [source][stage] free_heap=... free_8bit=... min_free_8bit=... largest_8bit=...

Deltas use free_heap only. Other fields are parsed when present and summarized
at block boundaries. Interleaved lines (e.g. [mem_MonitorTask][periodic]) are
ignored for the MGR tables.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from typing import Iterable, List, Optional, TextIO


ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")
MEM_LINE_RE = re.compile(
    r"ESP::MEM:\s*\[([^\]]+)\]\[([^\]]+)\]\s*"
    r"free_heap=(\d+)"
    r"(?:\s+free_8bit=(\d+))?"
    r"(?:\s+min_free_8bit=(\d+))?"
    r"(?:\s+largest_8bit=(\d+))?",
    re.IGNORECASE,
)
MODULE_DONE_RE = re.compile(r"^(mgr_init_module_done|mgr_run_module_done):\s*(\S+)\s*$")


@dataclass
class MemEvent:
    source: str
    stage: str
    free_heap: int
    line_no: int
    free_8bit: Optional[int] = None
    min_free_8bit: Optional[int] = None
    largest_8bit: Optional[int] = None


def strip_ansi(s: str) -> str:
    return ANSI_RE.sub("", s)


def parse_mem_events(lines: Iterable[str]) -> List[MemEvent]:
    events: List[MemEvent] = []
    for i, raw in enumerate(lines, start=1):
        line = strip_ansi(raw)
        m = MEM_LINE_RE.search(line)
        if not m:
            continue
        g4, g5, g6 = m.group(4), m.group(5), m.group(6)
        events.append(
            MemEvent(
                source=m.group(1).strip(),
                stage=m.group(2).strip(),
                free_heap=int(m.group(3)),
                line_no=i,
                free_8bit=int(g4) if g4 else None,
                min_free_8bit=int(g5) if g5 else None,
                largest_8bit=int(g6) if g6 else None,
            )
        )
    return events


def format_extra_metrics(e: MemEvent) -> Optional[str]:
    """Comma-separated free_8bit / min_free_8bit / largest_8bit when any are present."""
    parts: List[str] = []
    if e.free_8bit is not None:
        parts.append(f"free_8bit={e.free_8bit}")
    if e.min_free_8bit is not None:
        parts.append(f"min_free_8bit={e.min_free_8bit}")
    if e.largest_8bit is not None:
        parts.append(f"largest_8bit={e.largest_8bit}")
    return ", ".join(parts) if parts else None


def parse_module_stage(stage: str) -> Optional[tuple[str, str]]:
    m = MODULE_DONE_RE.match(stage)
    if not m:
        return None
    return m.group(1), m.group(2)


def find_init_block(events: List[MemEvent]) -> Optional[tuple[int, int]]:
    """Return (start_idx, end_idx_inclusive) for first mgr_init_begin .. mgr_init_done."""
    start = None
    for i, e in enumerate(events):
        if e.source == "MGR_Init" and e.stage == "mgr_init_begin":
            start = i
            break
    if start is None:
        return None
    for j in range(start, len(events)):
        e = events[j]
        if e.source == "MGR_Init" and e.stage == "mgr_init_done":
            return (start, j)
    return None


def find_run_block(events: List[MemEvent], after_index: int) -> Optional[tuple[int, int]]:
    """
    First mgr_run_begin after after_index, through last mgr_run_module_done before
    next mgr_run_begin (or end). If no second begin, slice until last run_module_done
    in that segment.
    """
    start = None
    for i in range(after_index + 1, len(events)):
        e = events[i]
        if e.source == "MGR_Run" and e.stage == "mgr_run_begin":
            start = i
            break
    if start is None:
        return None

    end = start
    for i in range(start + 1, len(events)):
        e = events[i]
        if e.source == "MGR_Run" and e.stage == "mgr_run_begin":
            break
        parsed = parse_module_stage(e.stage)
        if e.source == "MGR_Run" and parsed and parsed[0] == "mgr_run_module_done":
            end = i
    if end == start:
        return None
    return (start, end)


def table_row(module: str, heap_after: int, delta: int, width_mod: int) -> str:
    kb = delta / 1024.0
    return (
        f"| {module:<{width_mod}} | {heap_after:>9} | {delta:>10} | {kb:>8.2f} |"
    )


def print_init_table(events: List[MemEvent], start: int, end: int) -> None:
    rows: List[tuple[str, int, int]] = []
    prev_heap: Optional[int] = None
    width_mod = 8

    for i in range(start, end + 1):
        e = events[i]
        if e.source != "MGR_Init":
            continue
        if e.stage == "mgr_init_begin":
            prev_heap = e.free_heap
            continue
        parsed = parse_module_stage(e.stage)
        if parsed and parsed[0] == "mgr_init_module_done":
            mod = parsed[1]
            width_mod = max(width_mod, len(mod))
            if prev_heap is None:
                prev_heap = e.free_heap
                continue
            delta = prev_heap - e.free_heap
            rows.append((mod, e.free_heap, delta))
            prev_heap = e.free_heap

    if not rows:
        print("No mgr_init_module_done entries in init block.", file=sys.stderr)
        return

    final_heap = events[end].free_heap
    if prev_heap is None:
        total_delta = 0
    else:
        # Total from first begin to init_done heap
        begin_heap = events[start].free_heap
        total_delta = begin_heap - final_heap

    print("### MGR_Init (first `mgr_init_begin` .. `mgr_init_done`)\n")
    print(f"Baseline `mgr_init_begin` free_heap: **{events[start].free_heap}** bytes\n")
    print(f"| Module | free_heap after init | Δ from prev (B) | Δ (KiB) |")
    print(f"|--------|---------------------:|----------------:|--------:|")
    for mod, heap_after, delta in rows:
        print(table_row(mod, heap_after, delta, width_mod))
    sum_deltas = sum(r[2] for r in rows)
    print(f"| {'Σ (sum of step Δ)':<{width_mod}} | | **{sum_deltas}** | **{sum_deltas / 1024.0:.2f}** |")
    print(f"| {'Σ (begin → init_done)':<{width_mod}} | **{final_heap}** | **{total_delta}** | **{total_delta / 1024.0:.2f}** |")
    print()
    done_evt = events[end]
    extra = format_extra_metrics(done_evt)
    if extra:
        print(f"_At `mgr_init_done`: {extra}_\n")


def print_run_table(events: List[MemEvent], start: int, end: int) -> None:
    rows: List[tuple[str, int, int]] = []
    begin_heap = events[start].free_heap
    prev_heap = begin_heap
    width_mod = 8

    for i in range(start + 1, end + 1):
        e = events[i]
        if e.source != "MGR_Run":
            continue
        parsed = parse_module_stage(e.stage)
        if not parsed or parsed[0] != "mgr_run_module_done":
            continue
        mod = parsed[1]
        width_mod = max(width_mod, len(mod))
        delta = prev_heap - e.free_heap
        rows.append((mod, e.free_heap, delta))
        prev_heap = e.free_heap

    if not rows:
        print("No mgr_run_module_done entries after first `mgr_run_begin`.", file=sys.stderr)
        return

    total_delta = begin_heap - rows[-1][1]

    print("### MGR_Run (first full segment after `mgr_run_begin`)\n")
    print(f"Baseline `mgr_run_begin` free_heap: **{begin_heap}** bytes\n")
    print(f"| Module | free_heap after run | Δ from prev (B) | Δ (KiB) |")
    print(f"|--------|--------------------:|----------------:|--------:|")
    for mod, heap_after, delta in rows:
        print(table_row(mod, heap_after, delta, width_mod))
    sum_deltas = sum(r[2] for r in rows)
    print(f"| {'Σ (sum of step Δ)':<{width_mod}} | | **{sum_deltas}** | **{sum_deltas / 1024.0:.2f}** |")
    print(f"| {'Σ (begin → last module)':<{width_mod}} | **{rows[-1][1]}** | **{total_delta}** | **{total_delta / 1024.0:.2f}** |")
    print()
    tail_evt: Optional[MemEvent] = None
    for i in range(end, start, -1):
        e = events[i]
        if e.source != "MGR_Run":
            continue
        parsed = parse_module_stage(e.stage)
        if parsed and parsed[0] == "mgr_run_module_done":
            tail_evt = e
            break
    if tail_evt is not None:
        extra = format_extra_metrics(tail_evt)
        if extra:
            stage = tail_evt.stage
            print(f"_After last `mgr_run_module_done` (`{stage}`): {extra}_\n")


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Summarize ESP::MEM heap deltas per MGR module from monitor logs.",
    )
    ap.add_argument(
        "logfile",
        nargs="?",
        default="-",
        help="Path to log file, or '-' for stdin (default: stdin)",
    )
    args = ap.parse_args()

    if args.logfile == "-":
        fh: TextIO = sys.stdin
        lines = fh.readlines()
    else:
        with open(args.logfile, "r", encoding="utf-8", errors="replace") as fh:
            lines = fh.readlines()

    events = parse_mem_events(lines)
    if not events:
        print("No ESP::MEM lines with free_heap found.", file=sys.stderr)
        return 1

    init_range = find_init_block(events)
    if init_range is None:
        print("Could not find MGR_Init block (mgr_init_begin .. mgr_init_done).", file=sys.stderr)
        return 1

    print_init_table(events, init_range[0], init_range[1])

    run_range = find_run_block(events, init_range[1])
    if run_range is None:
        print("Could not find MGR_Run block after init.", file=sys.stderr)
        return 0

    print_run_table(events, run_range[0], run_range[1])
    return 0


if __name__ == "__main__":
    sys.exit(main())
