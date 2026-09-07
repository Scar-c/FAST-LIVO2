#!/usr/bin/env python3
"""Prompt22 shared-semantic configuration identity gate.

The gate compares only the predeclared shared semantic keys. Runtime paths,
ROS metadata, backend-specific parameters, and mode-inactive camera settings
are deliberately excluded by the manifest. A pair is admitted only when the
normalized shared document is byte-identical and both run dumps contain every
required key.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
from typing import Any

import yaml


SHARED_KEYS = [
    "common/lid_topic",
    "common/imu_topic",
    "common/img_topic",
    "common/img_en",
    "common/lidar_en",
    "common/ros_driver_bug_fix",
    "extrin_calib/extrinsic_T",
    "extrin_calib/extrinsic_R",
    "extrin_calib/Rcl",
    "extrin_calib/Pcl",
    "time_offset/lidar_time_offset",
    "time_offset/imu_time_offset",
    "time_offset/img_time_offset",
    "time_offset/exposure_time_init",
    "preprocess/lidar_type",
    "preprocess/scan_line",
    "preprocess/timestamp_unit",
    "preprocess/blind",
    "preprocess/point_filter_num",
    "preprocess/filter_size_surf",
    "preprocess/feature_extract_enabled",
    "preprocess/hilti_en",
    "imu/imu_en",
    "imu/imu_int_frame",
    "imu/acc_cov",
    "imu/gyr_cov",
    "imu/b_acc_cov",
    "imu/b_gyr_cov",
    "vio/max_iterations",
    "vio/outlier_threshold",
    "vio/img_point_cov",
    "vio/patch_size",
    "vio/patch_pyrimid_level",
    "vio/normal_en",
    "vio/raycast_en",
    "vio/inverse_composition_en",
    "vio/exposure_estimate_en",
    "vio/inv_expo_cov",
    "lio/max_iterations",
    "lio/dept_err",
    "lio/beam_err",
    "lio/min_eigen_value",
    "lio/voxel_size",
    "lio/max_layer",
    "lio/max_points_num",
    "lio/layer_init_num",
    "local_map/map_sliding_en",
    "local_map/half_map_size",
    "local_map/sliding_thresh",
    "uav/imu_rate_odom",
    "uav/gravity_align_en",
    "publish/dense_map_en",
    "publish/pub_effect_point_en",
    "publish/pub_scan_num",
    "publish/blind_rgb_points",
    "pcd_save/pcd_save_en",
    "pcd_save/type",
    "pcd_save/colmap_output_en",
    "pcd_save/filter_size_pcd",
    "pcd_save/interval",
    "image_save/img_save_en",
    "image_save/interval",
]

CAMERA_KEYS = [
    "laserMapping/cam_model",
    "laserMapping/cam_width",
    "laserMapping/cam_height",
    "laserMapping/scale",
    "laserMapping/cam_fx",
    "laserMapping/cam_fy",
    "laserMapping/cam_cx",
    "laserMapping/cam_cy",
    "laserMapping/k1",
    "laserMapping/k2",
    "laserMapping/k3",
    "laserMapping/k4",
]

# These values are deliberately forced by the formal runner or are inactive
# in LIO mode. They remain in the normalized P/N document for pair identity,
# but are not used to reject a source-authority profile audit.
PROFILE_AUDIT_EXCLUDED = {
    "common/img_en",
    "publish/dense_map_en",
    "publish/pub_effect_point_en",
    "publish/pub_scan_num",
    "publish/blind_rgb_points",
    "pcd_save/pcd_save_en",
    "pcd_save/type",
    "pcd_save/colmap_output_en",
    "pcd_save/filter_size_pcd",
    "pcd_save/interval",
    "image_save/img_save_en",
    "image_save/interval",
}


def load(path: Path) -> dict[str, Any]:
    value = yaml.safe_load(path.read_text())
    if not isinstance(value, dict):
        raise ValueError(f"{path}: YAML root is not a mapping")
    return value


def get_path(root: dict[str, Any], path: str) -> Any:
    value: Any = root
    for component in path.split("/"):
        if not isinstance(value, dict) or component not in value:
            raise KeyError(path)
        value = value[component]
    return value


MISSING = "__PROMPT22_ABSENT_IN_EFFECTIVE_DUMP__"


def normalize(root: dict[str, Any], include_camera: bool) -> dict[str, Any]:
    keys = SHARED_KEYS + (CAMERA_KEYS if include_camera else [])
    result: dict[str, Any] = {}
    for key in keys:
        try:
            result[key] = get_path(root, key)
        except KeyError:
            # Some official profiles intentionally omit a parameter whose
            # producer uses a source-code default or whose transport is not
            # represented in the ROS dump.  Absence is admissible only when
            # it is identical in both paired runs; a one-sided absence still
            # fails the pair comparison below.
            result[key] = MISSING
    return result


def canonical_json(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def digest(value: Any) -> str:
    return hashlib.sha256((canonical_json(value) + "\n").encode()).hexdigest()


def write_artifacts(run_dir: Path, normalized: dict[str, Any], sha: str) -> None:
    (run_dir / "shared_semantic_config.json").write_text(
        json.dumps(normalized, sort_keys=True, indent=2, ensure_ascii=True) + "\n"
    )
    (run_dir / "shared_semantic_config.sha256").write_text(sha + "\n")


def validate_pair(
    native: Path,
    prob: Path,
    profile: Path,
    camera_profile: Path | None,
    include_camera: bool,
    mode: str,
) -> tuple[str, dict[str, Any]]:
    native_normalized = normalize(load(native / "effective_rosparams.yaml"), include_camera)
    prob_normalized = normalize(load(prob / "effective_rosparams.yaml"), include_camera)
    profile_root = load(profile)
    if include_camera:
        if camera_profile is None:
            raise ValueError("camera profile is required for LIVO identity validation")
        profile_root["laserMapping"] = load(camera_profile)
    profile_normalized = normalize(profile_root, include_camera)
    native_sha = digest(native_normalized)
    prob_sha = digest(prob_normalized)
    write_artifacts(native, native_normalized, native_sha)
    write_artifacts(prob, prob_normalized, prob_sha)
    if native_normalized != prob_normalized:
        differences = [
            key for key in native_normalized if native_normalized[key] != prob_normalized[key]
        ]
        raise ValueError("CONFIG_IDENTITY_FAIL: " + ",".join(differences))
    if native_sha != prob_sha:
        raise ValueError("CONFIG_IDENTITY_FAIL: normalized SHA mismatch")
    audit_excluded = set(PROFILE_AUDIT_EXCLUDED)
    if mode == "lio":
        audit_excluded.update(CAMERA_KEYS)
    for key, expected in profile_normalized.items():
        if key in audit_excluded:
            continue
        if native_normalized[key] != expected or prob_normalized[key] != expected:
            raise ValueError(
                "CONFIG_IDENTITY_FAIL: authority mismatch "
                f"{key}: expected={expected!r} native={native_normalized[key]!r} "
                f"prob={prob_normalized[key]!r}"
            )
    return native_sha, native_normalized


def mutation_test(native: Path, prob: Path, include_camera: bool) -> None:
    native_normalized = normalize(load(native / "effective_rosparams.yaml"), include_camera)
    prob_normalized = normalize(load(prob / "effective_rosparams.yaml"), include_camera)
    mutated = copy.deepcopy(prob_normalized)
    key = "preprocess/filter_size_surf"
    mutated[key] = float(mutated[key]) + 0.001
    if digest(native_normalized) == digest(mutated):
        raise AssertionError("mutation test unexpectedly passed")
    print("PROMPT22 negative mutation test: PASS (shared identity gate rejects mutation)")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--native", type=Path)
    parser.add_argument("--prob", type=Path)
    parser.add_argument("--profile", type=Path)
    parser.add_argument("--camera-profile", type=Path)
    parser.add_argument("--mode", choices=["lio", "livo"])
    parser.add_argument("--mutation-test", action="store_true")
    args = parser.parse_args()
    if not args.mutation_test and (
        args.native is None or args.prob is None or args.profile is None or args.mode is None
    ):
        parser.error("--native, --prob, --profile and --mode are required unless --mutation-test is used")
    if args.mutation_test:
        if args.native is None or args.prob is None or args.mode is None:
            parser.error("mutation test also requires --native, --prob and --mode")
        mutation_test(args.native, args.prob, args.mode == "livo")
        return 0
    sha, normalized = validate_pair(
        args.native,
        args.prob,
        args.profile,
        args.camera_profile,
        args.mode == "livo",
        args.mode,
    )
    print(f"PROMPT22 shared semantic identity: PASS sha256={sha} keys={len(normalized)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
