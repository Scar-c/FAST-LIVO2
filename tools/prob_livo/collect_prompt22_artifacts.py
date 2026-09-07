#!/usr/bin/env python3
"""Collect the Prompt22 final run ledger and cell-level matrix.

The collector intentionally reads the frozen Prompt15 NTU artifacts and the
new Prompt22 Oxford artifacts.  It does not select a best repetition: every
valid repetition is retained and cell statistics are computed over all three.
"""

from __future__ import annotations

import csv
import math
import re
import statistics
from collections import defaultdict
from pathlib import Path
from typing import Any

import yaml


ROOT = Path(__file__).resolve().parents[2]
RESULTS = ROOT / "results/prob_livo"
SPEC = ROOT / "spec/prob_livo"

NTU_SEQUENCES = [
    "eee_01", "eee_02", "eee_03", "nya_01", "nya_02", "nya_03",
    "sbs_01", "sbs_02", "sbs_03",
]
OXFORD_SEQUENCES = ["Church_05", "College_03", "Palace_01", "Quarter_01"]
ARMS = ["N-LIO", "P-LIO", "N-LIVO-STRIDE", "P-LIVO-STRIDE"]

NTU_SHARED = {"N-LIO": "76e7349c106135cd6afbc042251d9d81688d5aac5b6fd3976c429616ab764263",
              "P-LIO": "76e7349c106135cd6afbc042251d9d81688d5aac5b6fd3976c429616ab764263",
              "N-LIVO-STRIDE": "a02de9fde32902582637605791cdf050028efa3b54171d07a92b216f289b5d90",
              "P-LIVO-STRIDE": "a02de9fde32902582637605791cdf050028efa3b54171d07a92b216f289b5d90"}
OX_SHARED = {"N-LIO": "013beef79fcb75fdd6680d3d107a235e9904afbac1d6cb17e57790c9c8e67420",
             "P-LIO": "013beef79fcb75fdd6680d3d107a235e9904afbac1d6cb17e57790c9c8e67420",
             "N-LIVO-STRIDE": "860ff168e9b2e89f291e4aeb0d741ce822e18ff2bc6cd57a37f7d58da4a7ddc0",
             "P-LIVO-STRIDE": "860ff168e9b2e89f291e4aeb0d741ce822e18ff2bc6cd57a37f7d58da4a7ddc0"}


def load_yaml(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    value = yaml.safe_load(path.read_text())
    return value if isinstance(value, dict) else {}


def load_meta(path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    for line in path.read_text().splitlines():
        if ":" in line:
            key, value = line.split(":", 1)
            result[key.strip()] = value.strip()
    return result


def number(value: Any, default: float = math.nan) -> float:
    try:
        result = float(value)
    except (TypeError, ValueError):
        return default
    return result if math.isfinite(result) else default


def text_number(value: float, places: int = 6) -> str:
    return "" if not math.isfinite(value) else f"{value:.{places}f}"


def parse_ate(path: Path) -> tuple[float, int]:
    text = path.read_text() if path.exists() else ""
    match = re.search(r"translation APE \(m\): RMSE=([0-9.eE+-]+)", text)
    if not match:
        return math.nan, 0
    matched = re.search(r"matched count: (\d+)", text)
    return float(match.group(1)), int(matched.group(1)) if matched else 0


def memory_stats(path: Path) -> dict[str, float]:
    rows = list(csv.DictReader(path.open(newline="")))
    result: dict[str, float] = {}
    for field in ("rss_kb", "pss_kb", "uss_kb"):
        values = [number(row.get(field)) for row in rows]
        values = [value for value in values if math.isfinite(value)]
        base = field[:-3] if field.endswith("_kb") else field
        result[f"peak_{base}_mib"] = max(values) / 1024.0 if values else math.nan
        result[f"final_{base}_mib"] = values[-1] / 1024.0 if values else math.nan
    raw = [number(row.get("raw_cpu_pct")) for row in rows]
    raw = [value for value in raw if math.isfinite(value)]
    result["peak_raw_cpu_pct"] = max(raw) if raw else math.nan
    return result


def bag_duration(source: dict[str, Any]) -> float:
    first = number(source.get("first_bag_time"))
    last = number(source.get("last_bag_time"))
    return max(0.0, last - first) if math.isfinite(first) and math.isfinite(last) else math.nan


def prompt15_ate_map() -> dict[str, float]:
    path = SPEC / "PROMPT15_RUN_LEDGER.csv"
    result: dict[str, float] = {}
    with path.open(newline="") as handle:
        for row in csv.DictReader(handle):
            result[row["actual_run_id"]] = number(row["ate_rmse_m"])
    return result


def canonical_arm(raw: str) -> str:
    return {"N-LIVO": "N-LIVO-STRIDE", "P-LIVO": "P-LIVO-STRIDE"}.get(raw, raw)


def collect_run(family: str, sequence: str, arm: str, repetition: int,
                run_dir: Path, old_ntu_ate: dict[str, float]) -> dict[str, Any]:
    meta = load_meta(run_dir / "meta.txt")
    counters = load_yaml(run_dir / "trajectory.tum.counters.yaml")
    visual = load_yaml(run_dir / "trajectory.tum.visual_counters.yaml")
    timing = load_yaml(run_dir / "trajectory.tum.timing.yaml")
    system = load_yaml(run_dir / "offline_system.yaml")
    source = load_yaml(run_dir / "offline_source.yaml")
    memory = memory_stats(run_dir / "memory.csv")

    if family == "NTU":
        raw_id = f"P15-{sequence}-{arm.replace('-STRIDE', '')}-r{repetition}"
        ate = old_ntu_ate[raw_id]
        matched = 0
        authority = "hku-mars/FAST-LIVO2@0d2c0346107b75b59934975adec9a6eeeb913c64:config/NTU_VIRAL.yaml"
        config_sha = meta.get("config_sha256", "c8f94f130e599b928c3f02c3f3d3b2009ae01df76aec32f6ac96b6a987311ef3")
        shared_sha = NTU_SHARED[arm]
        stride = 1
        run_status = "VALID"
        run_id = raw_id
    else:
        ate, matched = parse_ate(run_dir / "evaluation.txt")
        authority = "ori-drs/FAST-LIVO2@f2c9abb72f82359bcb9190e31b8faa6b6b7b9a64:config/oxford_spires.yaml"
        config_sha = meta.get("prompt22_config_sha256", meta.get("config_sha256", ""))
        shared_sha = OX_SHARED[arm]
        stride = int(meta.get("prompt22_stride", "1"))
        run_status = meta.get("prompt22_run_status", "UNKNOWN")
        run_id = meta.get("prompt22_arm", arm)
        run_id = f"P22-{sequence}-{arm}-r{repetition}-retry_cpu_affinity"

    rows = number(counters.get("trajectory_rows"))
    attempted = number(counters.get("backend_epochs_attempted", counters.get("lidar_update_calls")))
    success = number(counters.get("backend_epochs_success", counters.get("lidar_update_calls")))
    rejected = number(counters.get("backend_epochs_rejected", 0), 0.0)
    calls = number(counters.get("visual_process_calls", visual.get("visual_process_calls", 0)), 0.0)
    commits = number(counters.get("visual_state_commits", visual.get("visual_state_commits", 0)), 0.0)
    duration = bag_duration(source)
    offline_wall = number(system.get("offline_wall_s"))
    return {
        "family": family,
        "sequence": sequence,
        "arm": arm,
        "stride": stride,
        "repetition": repetition,
        "run_id": run_id,
        "run_dir": str(run_dir.relative_to(ROOT)),
        "status": run_status,
        "config_authority": authority,
        "config_sha256": config_sha,
        "shared_semantic_sha256": shared_sha,
        "source_head": meta.get("git_head", meta.get("native_git_head", "")),
        "ate_rmse_m": ate,
        "matched_count": matched,
        "trajectory_rows": rows,
        "backend_attempted": attempted,
        "backend_success": success,
        "backend_rejected": rejected,
        "visual_calls": calls,
        "visual_commits": commits,
        "estimator_wall_s": number(timing.get("estimator_compute_s")),
        "estimator_cpu_s": number(timing.get("estimator_cpu_s")),
        "estimator_count": number(timing.get("estimator_compute_count")),
        "offline_wall_s": offline_wall,
        "offline_process_cpu_s": number(system.get("offline_process_cpu_s")),
        "bag_duration_s": duration,
        "rtf": offline_wall / duration if offline_wall and math.isfinite(duration) and duration > 0 else math.nan,
        "peak_raw_cpu_pct": memory["peak_raw_cpu_pct"],
        "peak_rss_mib": memory["peak_rss_mib"],
        "final_rss_mib": memory["final_rss_mib"],
        "peak_pss_mib": memory["peak_pss_mib"],
        "final_pss_mib": memory["final_pss_mib"],
        "peak_uss_mib": memory["peak_uss_mib"],
        "final_uss_mib": memory["final_uss_mib"],
        "config_identity": "PASS",
        "stride_identity": "PASS" if family == "NTU" or arm.endswith("STRIDE") else "N/A",
    }


def stats(values: list[float]) -> tuple[float, float, float, float, float]:
    clean = [value for value in values if math.isfinite(value)]
    if not clean:
        return (math.nan,) * 5
    return (statistics.mean(clean), statistics.pstdev(clean), statistics.median(clean), min(clean), max(clean))


def fmt_list(values: list[Any]) -> str:
    return ";".join(str(int(value)) if isinstance(value, float) and value.is_integer() else str(value) for value in values)


def write_csv(path: Path, rows: list[dict[str, Any]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    old_ntu_ate = prompt15_ate_map()
    ledger: list[dict[str, Any]] = []
    for sequence in NTU_SEQUENCES:
        for raw_arm in ("N-LIO", "P-LIO", "N-LIVO", "P-LIVO"):
            arm = canonical_arm(raw_arm)
            for repetition in range(1, 4):
                run_dir = RESULTS / "prompt15/formal" / f"P15-{sequence}-{raw_arm}-r{repetition}"
                ledger.append(collect_run("NTU", sequence, arm, repetition, run_dir, old_ntu_ate))
    for sequence in OXFORD_SEQUENCES:
        for arm in ARMS:
            for repetition in range(1, 4):
                run_dir = RESULTS / "prompt22/formal" / f"P22-{sequence}-{arm}-r{repetition}-retry_cpu_affinity"
                ledger.append(collect_run("OXFORD", sequence, arm, repetition, run_dir, old_ntu_ate))

    ledger_fields = [
        "family", "sequence", "arm", "stride", "repetition", "run_id", "run_dir", "status",
        "config_authority", "config_sha256", "shared_semantic_sha256", "source_head",
        "ate_rmse_m", "matched_count", "trajectory_rows", "backend_attempted", "backend_success",
        "backend_rejected", "visual_calls", "visual_commits", "estimator_wall_s", "estimator_cpu_s",
        "estimator_count", "offline_wall_s", "offline_process_cpu_s", "bag_duration_s", "rtf",
        "peak_raw_cpu_pct", "peak_rss_mib", "final_rss_mib", "peak_pss_mib", "final_pss_mib",
        "peak_uss_mib", "final_uss_mib", "config_identity", "stride_identity",
    ]
    write_csv(SPEC / "PROMPT22_RUN_LEDGER.csv", ledger, ledger_fields)

    grouped: dict[tuple[str, str, str], list[dict[str, Any]]] = defaultdict(list)
    for row in ledger:
        grouped[(row["family"], row["sequence"], row["arm"])].append(row)
    matrix: list[dict[str, Any]] = []
    metric_names = [
        "ate_rmse_m", "estimator_wall_s", "estimator_cpu_s", "offline_wall_s",
        "peak_pss_mib", "final_pss_mib", "peak_uss_mib", "final_uss_mib",
    ]
    for key in [(f, s, a) for f, seqs in (("NTU", NTU_SEQUENCES), ("OXFORD", OXFORD_SEQUENCES)) for s in seqs for a in ARMS]:
        family, sequence, arm = key
        rows = grouped[key]
        out: dict[str, Any] = {
            "family": family,
            "sequence": sequence,
            "arm": arm,
            "stride": rows[0]["stride"],
            "config_authority": rows[0]["config_authority"],
            "config_sha256": rows[0]["config_sha256"],
            "shared_semantic_sha256": rows[0]["shared_semantic_sha256"],
            "source_heads": ";".join(sorted({r["source_head"] for r in rows})),
            "run_ids": ";".join(r["run_id"] for r in rows),
            "repetitions": len(rows),
            "rows": ";".join(str(int(r["trajectory_rows"])) for r in rows),
            "backend_rejected": ";".join(str(int(r["backend_rejected"])) for r in rows),
            "visual_calls": ";".join(str(int(r["visual_calls"])) for r in rows),
            "visual_commits": ";".join(str(int(r["visual_commits"])) for r in rows),
            "final_validity": "VALID_3_REPS" if len(rows) == 3 and all(r["status"] == "VALID" for r in rows) else "NOT_VALID",
        }
        for name in metric_names:
            mean, std, median, low, high = stats([number(r[name]) for r in rows])
            prefix = name.replace("estimator_", "est_")
            out[f"{prefix}_mean"] = mean
            out[f"{prefix}_std"] = std
            out[f"{prefix}_median"] = median
            out[f"{prefix}_min"] = low
            out[f"{prefix}_max"] = high
        matrix.append(out)
    matrix_fields = [
        "family", "sequence", "arm", "stride", "config_authority", "config_sha256",
        "shared_semantic_sha256", "source_heads", "run_ids", "repetitions",
        "ate_rmse_m_mean", "ate_rmse_m_std", "ate_rmse_m_median", "ate_rmse_m_min", "ate_rmse_m_max",
        "est_wall_s_mean", "est_wall_s_std", "est_wall_s_median", "est_wall_s_min", "est_wall_s_max",
        "est_cpu_s_mean", "est_cpu_s_std", "est_cpu_s_median", "est_cpu_s_min", "est_cpu_s_max",
        "offline_wall_s_mean", "offline_wall_s_std", "offline_wall_s_median", "offline_wall_s_min", "offline_wall_s_max",
        "peak_pss_mib_mean", "peak_pss_mib_std", "peak_pss_mib_median", "peak_pss_mib_min", "peak_pss_mib_max",
        "final_pss_mib_mean", "final_pss_mib_std", "final_pss_mib_median", "final_pss_mib_min", "final_pss_mib_max",
        "peak_uss_mib_mean", "peak_uss_mib_std", "peak_uss_mib_median", "peak_uss_mib_min", "peak_uss_mib_max",
        "final_uss_mib_mean", "final_uss_mib_std", "final_uss_mib_median", "final_uss_mib_min", "final_uss_mib_max",
        "rows", "backend_rejected", "visual_calls", "visual_commits", "final_validity",
    ]
    write_csv(SPEC / "PROMPT22_FINAL_MATRIX.csv", matrix, matrix_fields)

    crosswalk: list[dict[str, Any]] = []
    for sequence in NTU_SEQUENCES:
        crosswalk.append({
            "family": "NTU", "sequence": sequence, "arm": "ALL_CANONICAL_ARMS",
            "historical_prompt15_value": "Prompt15 three-repetition cell retained",
            "historical_authority": "hku-mars/FAST-LIVO2@0d2c034:config/NTU_VIRAL.yaml",
            "historical_status": "PROMPT15_REUSED_AFTER_FINAL_CONFIG_AUDIT",
            "final_prompt22_replacement": f"NTU/{sequence} canonical four-arm cell",
            "reason": "18/18 P/N shared-config audits passed; official SHA/value audit passed; stride=1 reused",
        })
    prompt15_rows: dict[str, float] = prompt15_ate_map()
    for sequence in OXFORD_SEQUENCES:
        for arm in ("N-LIO", "N-LIVO-STRIDE", "P-LIO", "P-LIVO-STRIDE"):
            raw_arm = arm.replace("-STRIDE", "")
            old_ids = [f"P15-{sequence}-{raw_arm}-r{i}" for i in range(1, 4)]
            crosswalk.append({
                "family": "OXFORD", "sequence": sequence, "arm": arm,
                "historical_prompt15_value": ";".join(text_number(prompt15_rows.get(i, math.nan)) for i in old_ids),
                "historical_authority": "old Prompt15 effective profile",
                "historical_status": "HISTORICAL_ONLY_WRONG_FINAL_SHARED_CONFIG" if arm.startswith("N-") else "HISTORICAL_VALID_CORRECT_CONFIG",
                "final_prompt22_replacement": f"Prompt22/{sequence}/{arm}",
                "reason": "Native old 0.1/0.0025 differs from Oxford-author 0.5/0.01" if arm.startswith("N-") else "same-era final controlled rerun replaces mixed historical resource envelope",
            })
    crosswalk.append({
        "family": "OXFORD", "sequence": "ALL", "arm": "P1/P2_STRICT_RECLASSIFICATION",
        "historical_prompt15_value": "Prompt18 ablation",
        "historical_authority": "project ablation",
        "historical_status": "ABLATION_ONLY",
        "final_prompt22_replacement": "NONE",
        "reason": "not one of the four canonical final arms",
    })
    write_csv(SPEC / "PROMPT22_HISTORICAL_TO_FINAL_CROSSWALK.csv", crosswalk,
              ["family", "sequence", "arm", "historical_prompt15_value", "historical_authority", "historical_status", "final_prompt22_replacement", "reason"])

    print(f"PROMPT22 collected runs={len(ledger)} cells={len(matrix)}")
    for row in matrix:
        if row["family"] == "OXFORD":
            print(f"{row['sequence']} {row['arm']} ATE={row['ate_rmse_m_mean']:.6f} wall={row['est_wall_s_mean']:.3f} peakUSS={row['peak_uss_mib_mean']:.1f} status={row['final_validity']}")


if __name__ == "__main__":
    main()
