#!/usr/bin/env python3
"""Decompose the legacy/current 58-IMU initialization numeric seam.

This is a bounded diagnostic, not a production runtime path.  The four
variants intentionally keep the two recurrence operation orders separate.
"""

import argparse
import json
import math

import numpy as np
import rosbag


GRAVITY_NORM = 9.7946
SAMPLE_COUNT = 58


def load_samples(path):
    samples = []
    with rosbag.Bag(path, "r") as bag:
        for _, message, _ in bag.read_messages(topics=["/imu/imu"]):
            samples.append(
                (
                    np.array(
                        [
                            message.linear_acceleration.x,
                            message.linear_acceleration.y,
                            message.linear_acceleration.z,
                        ],
                        dtype=np.float64,
                    ),
                    np.array(
                        [
                            message.angular_velocity.x,
                            message.angular_velocity.y,
                            message.angular_velocity.z,
                        ],
                        dtype=np.float64,
                    ),
                    message.header.stamp.to_sec(),
                )
            )
            if len(samples) == SAMPLE_COUNT:
                break
    if len(samples) != SAMPLE_COUNT:
        raise RuntimeError(f"expected {SAMPLE_COUNT} IMU samples, got {len(samples)}")
    return samples


def mean_variant(samples, dtype, recurrence):
    acceleration = np.zeros(3, dtype=dtype)
    gyro = np.zeros(3, dtype=dtype)
    for index, (sample_acceleration, sample_gyro, _) in enumerate(samples, 1):
        acceleration_sample = sample_acceleration.astype(dtype)
        gyro_sample = sample_gyro.astype(dtype)
        count = dtype(index)
        if recurrence == "legacy":
            acceleration = (
                acceleration + (acceleration_sample - acceleration) / count
            ).astype(dtype)
            gyro = (gyro + (gyro_sample - gyro) / count).astype(dtype)
        elif recurrence == "migrated":
            previous_count = dtype(index - 1)
            acceleration = (
                (acceleration * previous_count + acceleration_sample) / count
            ).astype(dtype)
            gyro = ((gyro * previous_count + gyro_sample) / count).astype(dtype)
        else:
            raise ValueError(recurrence)

    norm = np.linalg.norm(acceleration)
    if dtype == np.float32:
        scale = np.float32(GRAVITY_NORM / float(norm))
        gravity = (-acceleration * np.float32(GRAVITY_NORM) / norm).astype(dtype)
    else:
        scale = float(GRAVITY_NORM / norm)
        gravity = -acceleration * GRAVITY_NORM / norm
    return {
        "mean_acc": acceleration,
        "mean_gyro": gyro,
        "acc_norm": norm,
        "imu_scale": scale,
        "initial_gravity": gravity,
    }


def normalize(vector):
    return vector / np.linalg.norm(vector)


def from_two_vectors(first, second, dtype):
    first = normalize(first.astype(dtype))
    second = normalize(second.astype(dtype))
    cross = np.cross(first, second).astype(dtype)
    dot = dtype(np.dot(first, second))
    quaternion = np.array(
        [dtype(1.0) + dot, cross[0], cross[1], cross[2]], dtype=dtype
    )
    return quaternion / np.linalg.norm(quaternion)


def quaternion_matrix(quaternion, dtype):
    w, x, y, z = quaternion
    two = dtype(2.0)
    return np.array(
        [
            [1 - two * (y * y + z * z), two * (x * y - z * w), two * (x * z + y * w)],
            [two * (x * y + z * w), 1 - two * (x * x + z * z), two * (y * z - x * w)],
            [two * (x * z - y * w), two * (y * z + x * w), 1 - two * (x * x + y * y)],
        ],
        dtype=dtype,
    )


def gravity_rotation(mean_acceleration, dtype):
    gravity_norm = dtype(GRAVITY_NORM)
    gravity = (-mean_acceleration * gravity_norm / np.linalg.norm(mean_acceleration)).astype(
        dtype
    )
    reference = np.array([0, 0, -gravity_norm], dtype=dtype)
    quaternion = from_two_vectors(gravity, reference, dtype)
    aligned = quaternion_matrix(quaternion, dtype)
    first_column = aligned[:, 0]
    yaw = math.atan2(float(first_column[1]), float(first_column[0]))
    angle = dtype(-yaw)
    cosine = dtype(math.cos(float(angle)))
    sine = dtype(math.sin(float(angle)))
    yaw_inverse = np.array(
        [
            [cosine, -sine, dtype(0)],
            [sine, cosine, dtype(0)],
            [dtype(0), dtype(0), dtype(1)],
        ],
        dtype=dtype,
    )
    return (yaw_inverse @ aligned).astype(dtype)


def serialise(value):
    if isinstance(value, np.ndarray):
        return [serialise(item) for item in value.tolist()]
    if isinstance(value, (np.floating, float)):
        return float(value)
    return value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("bag")
    args = parser.parse_args()
    samples = load_samples(args.bag)
    variants = {
        "M1": mean_variant(samples, np.float32, "legacy"),
        "M2": mean_variant(samples, np.float32, "migrated"),
        "M3": mean_variant(samples, np.float64, "legacy"),
        "M4": mean_variant(samples, np.float64, "migrated"),
    }
    legacy = variants["M1"]
    for variant in variants.values():
        variant["delta_to_M1_acc"] = np.linalg.norm(
            variant["mean_acc"].astype(np.float64) - legacy["mean_acc"].astype(np.float64)
        )
        variant["delta_to_M1_gyro"] = np.linalg.norm(
            variant["mean_gyro"].astype(np.float64) - legacy["mean_gyro"].astype(np.float64)
        )
        variant["delta_to_M1_scale"] = abs(
            float(variant["imu_scale"]) - float(legacy["imu_scale"])
        )

    def delta(left, right, key):
        return float(
            np.linalg.norm(
                variants[left][key].astype(np.float64)
                - variants[right][key].astype(np.float64)
            )
        )

    recurrence_float = {
        "acc": delta("M1", "M2", "mean_acc"),
        "gyro": delta("M1", "M2", "mean_gyro"),
        "scale": abs(float(variants["M1"]["imu_scale"]) - float(variants["M2"]["imu_scale"])),
    }
    recurrence_double = {
        "acc": delta("M3", "M4", "mean_acc"),
        "gyro": delta("M3", "M4", "mean_gyro"),
        "scale": abs(float(variants["M3"]["imu_scale"]) - float(variants["M4"]["imu_scale"])),
    }
    scalar_width_legacy = {
        "acc": delta("M1", "M3", "mean_acc"),
        "gyro": delta("M1", "M3", "mean_gyro"),
        "scale": abs(float(variants["M1"]["imu_scale"]) - float(variants["M3"]["imu_scale"])),
    }
    scalar_width_migrated = {
        "acc": delta("M2", "M4", "mean_acc"),
        "gyro": delta("M2", "M4", "mean_gyro"),
        "scale": abs(float(variants["M2"]["imu_scale"]) - float(variants["M4"]["imu_scale"])),
    }

    common_mean = variants["M4"]["mean_acc"]
    rotation_float = gravity_rotation(common_mean.astype(np.float32), np.float32).astype(
        np.float64
    )
    rotation_double = gravity_rotation(common_mean, np.float64)
    rotation_delta = rotation_float.T @ rotation_double
    cosine = np.clip((np.trace(rotation_delta) - 1.0) / 2.0, -1.0, 1.0)
    rotation_log_norm = math.acos(float(cosine))
    p18_float = np.eye(18, dtype=np.float32) * np.float32(1e-4)
    p18_double = np.eye(18, dtype=np.float64) * 1e-4
    rotation_variance_float = np.float32(0.1 / 180.0 * math.pi)
    rotation_variance_double = 0.1 / 180.0 * math.pi
    p18_float[:3, :3] = np.eye(3, dtype=np.float32) * rotation_variance_float
    p18_double[:3, :3] = np.eye(3, dtype=np.float64) * rotation_variance_double

    result = {
        "sample_count": len(samples),
        "last_imu_timestamp": samples[-1][2],
        "variants": {
            name: {key: serialise(value) for key, value in variant.items()}
            for name, variant in variants.items()
        },
        "so3_split": {
            "rotation_float": serialise(rotation_float),
            "rotation_double": serialise(rotation_double),
            "log_rotation_delta_norm": rotation_log_norm,
            "gravity_float": serialise(variants["M4"]["initial_gravity"].astype(np.float32)),
            "gravity_double": serialise(variants["M4"]["initial_gravity"]),
            "initial_bg_float": serialise(variants["M1"]["mean_gyro"]),
            "initial_bg_double": serialise(variants["M4"]["mean_gyro"]),
            "p18_max_abs_delta": float(
                np.max(np.abs(p18_float.astype(np.float64) - p18_double))
            ),
        },
        "contributions": {
            "scalar_width_legacy_recurrence": scalar_width_legacy,
            "scalar_width_migrated_recurrence": scalar_width_migrated,
            "recurrence_float": recurrence_float,
            "recurrence_double": recurrence_double,
        },
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
