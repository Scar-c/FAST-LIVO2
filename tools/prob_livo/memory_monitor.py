#!/usr/bin/env python3
"""Sample one estimator process from /proc without touching the estimator."""

import argparse
import csv
import os
import time
from datetime import datetime, timezone


STATUS_FIELDS = (
    "VmRSS",
    "VmHWM",
    "VmSize",
    "RssAnon",
    "RssFile",
    "VmSwap",
)
SMAPS_FIELDS = (
    "Rss",
    "Pss",
    "Private_Clean",
    "Private_Dirty",
    "Shared_Clean",
    "Shared_Dirty",
    "Swap",
)


def read_kb_file(path, fields):
    values = {field: "" for field in fields}
    with open(path, "r", encoding="utf-8") as stream:
        for line in stream:
            name, _, remainder = line.partition(":")
            if name not in values:
                continue
            value = remainder.strip().split()[0]
            values[name] = int(value)
    return values


def sample(pid):
    proc = f"/proc/{pid}"
    values = {"pid": pid}
    values.update(read_kb_file(f"{proc}/status", STATUS_FIELDS))
    try:
        values.update(read_kb_file(f"{proc}/smaps_rollup", SMAPS_FIELDS))
    except FileNotFoundError:
        values.update({field: "" for field in SMAPS_FIELDS})
    private_clean = values["Private_Clean"] or 0
    private_dirty = values["Private_Dirty"] or 0
    values["USS"] = private_clean + private_dirty
    return values


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--csv", required=True)
    parser.add_argument("--label", default="")
    parser.add_argument("--interval", type=float, default=2.0)
    args = parser.parse_args()
    if args.interval <= 0:
        parser.error("--interval must be positive")

    parent = os.path.dirname(os.path.abspath(args.csv))
    os.makedirs(parent, exist_ok=True)
    fieldnames = [
        "sample_utc",
        "elapsed_s",
        "label",
        "pid",
        *STATUS_FIELDS,
        *SMAPS_FIELDS,
        "USS",
    ]
    start = time.monotonic()
    sampled = False
    with open(args.csv, "w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        stream.flush()
        while True:
            try:
                values = sample(args.pid)
            except (FileNotFoundError, ProcessLookupError):
                if sampled:
                    break
                if time.monotonic() - start > 30.0:
                    return 2
                time.sleep(min(args.interval, 0.25))
                continue
            row = {
                "sample_utc": datetime.now(timezone.utc).isoformat(),
                "elapsed_s": f"{time.monotonic() - start:.3f}",
                "label": args.label,
                **values,
            }
            writer.writerow(row)
            stream.flush()
            sampled = True
            time.sleep(args.interval)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
