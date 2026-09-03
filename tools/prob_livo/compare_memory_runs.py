#!/usr/bin/env python3
"""Summarize Prompt10 memory CSVs using the prescribed 20--40 s baseline."""

import argparse
import csv
import json
import statistics
from pathlib import Path


METRICS = (
    "VmRSS",
    "VmHWM",
    "VmSize",
    "RssAnon",
    "RssFile",
    "VmSwap",
    "Rss",
    "Pss",
    "Private_Clean",
    "Private_Dirty",
    "Shared_Clean",
    "Shared_Dirty",
    "Swap",
    "USS",
)


def read_meta(path):
    meta = {}
    if not path.exists():
        return meta
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition(":")
        if separator:
            meta[key.strip()] = value.strip()
    return meta


def read_memory(path):
    rows = []
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            converted = {"elapsed_s": float(row["elapsed_s"])}
            for metric in METRICS:
                value = row.get(metric, "")
                converted[metric] = float(value) if value != "" else None
            rows.append(converted)
    return rows


def median_metric(rows, metric):
    values = [row[metric] for row in rows if row[metric] is not None]
    return statistics.median(values) if values else None


def summarize(run_dir):
    run_dir = Path(run_dir)
    memory_path = run_dir / "memory.csv"
    rows = read_memory(memory_path)
    early = [row for row in rows if 20.0 <= row["elapsed_s"] <= 40.0]
    last = rows[-1] if rows else {}
    summary = {
        "run_dir": str(run_dir),
        "run_id": run_dir.name,
        "meta": read_meta(run_dir / "meta.txt"),
        "samples": len(rows),
        "duration_s": last.get("elapsed_s"),
        "early_window_s": [20, 40],
        "early_samples": len(early),
        "early_median_kb": {metric: median_metric(early, metric) for metric in METRICS},
        "end_kb": {metric: last.get(metric) for metric in METRICS},
        "peak_kb": {},
        "peak_elapsed_s": {},
        "growth_end_minus_early_kb": {},
    }
    for metric in METRICS:
        valid = [row for row in rows if row[metric] is not None]
        if valid:
            peak = max(valid, key=lambda row: row[metric])
            summary["peak_kb"][metric] = peak[metric]
            summary["peak_elapsed_s"][metric] = peak["elapsed_s"]
        else:
            summary["peak_kb"][metric] = None
            summary["peak_elapsed_s"][metric] = None
        early_value = summary["early_median_kb"][metric]
        end_value = summary["end_kb"][metric]
        summary["growth_end_minus_early_kb"][metric] = (
            end_value - early_value
            if end_value is not None and early_value is not None
            else None
        )
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run_dirs", nargs="+", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = {"runs": [summarize(run_dir) for run_dir in args.run_dirs]}
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")


if __name__ == "__main__":
    main()
