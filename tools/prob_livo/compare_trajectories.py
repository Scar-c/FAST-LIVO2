#!/usr/bin/env python3
"""Compare raw TUM trajectories without a mutual SE(3) alignment."""

import argparse
import json
from pathlib import Path

import numpy as np


def load(path):
    rows = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        fields = line.split()
        if not fields or fields[0].startswith("#"):
            continue
        if len(fields) < 8:
            raise ValueError(f"{path}: expected 8 TUM columns")
        rows.append([float(value) for value in fields[:8]])
    result = np.asarray(rows, dtype=float)
    if len(result) < 2 or not np.all(np.isfinite(result)):
        raise ValueError(f"{path}: invalid trajectory")
    if np.any(np.diff(result[:, 0]) <= 0):
        raise ValueError(f"{path}: timestamps are not strictly increasing")
    return result


def normalize(q):
    return q / np.linalg.norm(q, axis=1, keepdims=True)


def rotation_delta(q0, q1):
    q0 = normalize(q0)
    q1 = normalize(q1)
    # Quaternion dot products identify the shorter relative rotation.
    dot = np.abs(np.sum(q0 * q1, axis=1))
    return 2.0 * np.arccos(np.clip(dot, -1.0, 1.0))


def compare(reference, estimate):
    ref = load(reference)
    est = load(estimate)
    lo = max(ref[0, 0], est[0, 0])
    hi = min(ref[-1, 0], est[-1, 0])
    if hi <= lo:
        raise ValueError("trajectories have no timestamp overlap")
    selected = (est[:, 0] >= lo) & (est[:, 0] <= hi)
    est = est[selected]
    positions = np.column_stack(
        [np.interp(est[:, 0], ref[:, 0], ref[:, axis]) for axis in (1, 2, 3)]
    )
    nearest = np.searchsorted(ref[:, 0], est[:, 0]).clip(1, len(ref) - 1)
    left = nearest - 1
    use_right = np.abs(ref[nearest, 0] - est[:, 0]) < np.abs(
        ref[left, 0] - est[:, 0]
    )
    indices = np.where(use_right, nearest, left)
    time_delta = est[:, 0] - ref[indices, 0]
    translation_delta = est[:, 1:4] - positions
    rotation_delta_rad = rotation_delta(est[:, 4:8], ref[indices, 4:8])
    translation_norm = np.linalg.norm(translation_delta, axis=1)
    translation_rmse = float(np.sqrt(np.mean(translation_norm**2)))
    rotation_rmse = float(np.sqrt(np.mean(rotation_delta_rad**2)))
    if (
        len(est) == len(load(estimate))
        and translation_norm.max() <= 1e-6
        and rotation_delta_rad.max() <= 1e-6
    ):
        classification = "I3_TRAJECTORY_EQUIVALENT"
    elif translation_rmse <= 0.5 and rotation_rmse <= 0.5:
        classification = "I3_TRAJECTORY_CLOSE_NONIDENTICAL"
    else:
        classification = "I3_SEMANTIC_MISMATCH"
    return {
        "reference": str(Path(reference).resolve()),
        "estimate": str(Path(estimate).resolve()),
        "reference_rows": int(len(ref)),
        "estimate_rows": int(len(load(estimate))),
        "matched_rows": int(len(est)),
        "timestamp_overlap_s": [float(lo), float(hi)],
        "timestamp_delta_s": {
            "rmse": float(np.sqrt(np.mean(time_delta**2))),
            "median_abs": float(np.median(np.abs(time_delta))),
            "max_abs": float(np.max(np.abs(time_delta))),
        },
        "translation_delta_m": {
            "rmse": translation_rmse,
            "median": float(np.median(translation_norm)),
            "max": float(np.max(translation_norm)),
        },
        "rotation_delta_rad": {
            "rmse": rotation_rmse,
            "median": float(np.median(rotation_delta_rad)),
            "max": float(np.max(rotation_delta_rad)),
        },
        "alignment": "NONE_RAW_WORLD_FRAMES",
        "classification": classification,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference")
    parser.add_argument("estimate")
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    result = compare(args.reference, args.estimate)
    Path(args.out).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
