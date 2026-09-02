#!/usr/bin/env python3
"""Compare TUM trajectories by monotonic near-exact timestamp pairing.

The primary metric intentionally does not interpolate either trajectory.  A
pair is accepted only when the two source timestamps differ by --tolerance.
"""

import argparse
import json
import math


def load(path):
    rows = []
    with open(path, "r", encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            fields = line.split()
            if not fields or fields[0].startswith("#"):
                continue
            if len(fields) != 8:
                raise ValueError(f"{path}:{line_number}: expected 8 TUM fields")
            values = [float(value) for value in fields]
            if not all(math.isfinite(value) for value in values):
                raise ValueError(f"{path}:{line_number}: non-finite value")
            rows.append(values)
    if any(rows[index][0] > rows[index + 1][0]
           for index in range(len(rows) - 1)):
        raise ValueError(f"{path}: timestamps are not monotonic")
    return rows


def rms(values):
    return math.sqrt(sum(value * value for value in values) / len(values)) if values else None


def summary(values):
    if not values:
        return {"rmse": None, "median": None, "max": None}
    ordered = sorted(values)
    middle = len(ordered) // 2
    median = ordered[middle] if len(ordered) % 2 else 0.5 * (
        ordered[middle - 1] + ordered[middle])
    return {"rmse": rms(values), "median": median, "max": max(values)}


def quaternion_angle(first, second):
    # q_delta = conjugate(first) * second. The absolute scalar part makes
    # q and -q equivalent, as required for the same physical rotation.
    ax, ay, az, aw = first[4:8]
    bx, by, bz, bw = second[4:8]
    an = math.sqrt(ax * ax + ay * ay + az * az + aw * aw)
    bn = math.sqrt(bx * bx + by * by + bz * bz + bw * bw)
    ax, ay, az, aw = ax / an, ay / an, az / an, aw / an
    bx, by, bz, bw = bx / bn, by / bn, bz / bn, bw / bn
    scalar = abs(aw * bw + ax * bx + ay * by + az * bz)
    return 2.0 * math.acos(max(-1.0, min(1.0, scalar)))


def compare(old, new, tolerance):
    old_index = 0
    new_index = 0
    pairs = []
    unmatched_old = 0
    unmatched_new = 0
    timestamp_deltas = []
    translation_errors = []
    rotation_errors = []
    while old_index < len(old) and new_index < len(new):
        delta = new[new_index][0] - old[old_index][0]
        if abs(delta) <= tolerance:
            reference = old[old_index]
            estimate = new[new_index]
            pairs.append({"old_index": old_index, "new_index": new_index,
                          "old_timestamp": reference[0],
                          "new_timestamp": estimate[0], "dt_s": delta})
            timestamp_deltas.append(delta)
            translation_errors.append(math.sqrt(sum(
                (estimate[axis] - reference[axis]) ** 2 for axis in (1, 2, 3))))
            rotation_errors.append(quaternion_angle(reference, estimate))
            old_index += 1
            new_index += 1
        elif old[old_index][0] < new[new_index][0]:
            unmatched_old += 1
            old_index += 1
        else:
            unmatched_new += 1
            new_index += 1
    unmatched_old += len(old) - old_index
    unmatched_new += len(new) - new_index
    return {
        "pairing": "monotonic_near_exact_timestamp_no_interpolation",
        "timestamp_tolerance_s": tolerance,
        "old_rows": len(old),
        "new_rows": len(new),
        "paired_rows": len(pairs),
        "unmatched_old_rows": unmatched_old,
        "unmatched_new_rows": unmatched_new,
        "timestamp_delta_s": summary([abs(value) for value in timestamp_deltas]),
        "translation_error_m": summary(translation_errors),
        "rotation_error_rad": summary(rotation_errors),
        "pairs": pairs,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("old")
    parser.add_argument("new")
    parser.add_argument("--tolerance", type=float, default=1e-6)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    result = compare(load(args.old), load(args.new), args.tolerance)
    with open(args.out, "w", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps({key: value for key, value in result.items() if key != "pairs"},
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
