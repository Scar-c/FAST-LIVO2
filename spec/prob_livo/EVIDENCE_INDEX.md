# Prob-LIVO Evidence Index

Prompt 0 evidence is source identity, source audit, build output, and file
inventory only. No bag-run or accuracy evidence is claimed. Prompt 1 evidence
adds deterministic filter-core parity and host-build gates. Prompt 2 evidence
adds the terminal callback corrective and deterministic Super-native
IMU/undistortion seam tests; it still claims no bag-run evidence.

## Repository identity

| Evidence | Value |
|---|---|
| Host path | `/home/lc/super_livo/src/FAST-LIVO2` |
| Host initial branch | `main` |
| Host baseline SHA | `0d2c0346107b75b59934975adec9a6eeeb913c64` |
| Integration branch | `prob-livo`, created from host `main` |
| Host origin | `https://github.com/Scar-c/FAST-LIVO2.git` |
| Host upstream | none configured |
| Prob-LIO reference | `/home/lc/super_livo/ref/Super-LIO` |
| Reference branch | `prob-lio` |
| Reference SHA | `9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e` |
| Legacy workspace | `/home/lc/prob_lio/src/Super-LIO` |
| Dataset root | `/home/lc/super_livo/bag` |

Both host and reference worktrees were clean at inspection. The reference was
not modified. The host integration branch had no pre-existing work when
created.

## Build evidence

Environment:

```text
ROS             noetic
catkin          0.8.12
compiler        g++ 9.4.0
CMake           3.18.4
Eigen           3.3.7
PCL             1.10.0
OpenCV          4.2.0
vikit_common    0.0.0 (from existing /home/lc/design_ws/devel overlay)
vikit_ros       0.0.0 (from existing /home/lc/design_ws/devel overlay)
```

Command and result:

```bash
source /home/lc/design_ws/devel/setup.bash
catkin_make --pkg fast_livo
```

Return code: `0`. Result: `fastlivo_mapping` and all host libraries reached
`100%` and were linked under the workspace devel directory. A preliminary
unoverlaid invocation returned `1` at `find_package(vikit_common)`; no source
change was made to address it.

## Source audit anchors

The complete ownership audit is in `SPEC.md`. Primary anchors are:

```text
FAST-LIVO2 scheduler: src/main.cpp:3-10; src/LIVMapper.cpp:534-552,884-1030
FAST-LIVO2 state/type: include/common_lib.h:102-206
FAST-LIVO2 IMU: src/LIVMapper.cpp:248-265; src/IMU_Processing.cpp:237-588
FAST-LIVO2 LIO: src/LIVMapper.cpp:336-430; src/voxel_map.cpp:15-135,338-786
FAST-LIVO2 VIO: include/vio.h:83-167; src/vio.cpp:352-602,804-1100,1398-1680,1786-1854
Prob-LIO ESKF: src/super_lio/include/lio/ESKF.h:12-123; src/lio/ESKF.cpp:187-336
Prob-LIO frontend: src/lio/super_lio.cpp:384-555
Prob-LIO OctVox/HKNN: include/OctVoxMap/OctVoxMap.hpp:104-210,417-553
Prob-LIO QR/P3: include/lio/prob_qr_plane.h:40-190
Prob-LIO P1/P2/P4: include/lio/point_covariance.h:37-64,115-152,236-366
Prob-LIO legacy association: src/lio/super_lio.cpp:48-54,800-890
Prob-LIO experimental P5: include/lio/point_covariance.h:369-601; src/lio/super_lio.cpp:823-840
```

## Dataset inventory evidence

See `../../results/prob_livo/README.md` for the complete lightweight listing
of NTU/OXFORD bags, metadata, sizes, and future stage notes. Camera calibration
files are present in the inventory. Camera-topic presence remains pending an
explicit rosbag topic audit; no bag was run.

## Prompt 0 commit evidence

The two focused I0 commits are:

```text
I0 bootstrap commit: 7dac83e726a32bc2a8551f445322959a523cbba3
I0 final/push-record commit: 9ed486cc9e78f075ec74f3c9c48eb2a0efcc0c1b
I0 final HEAD: 9ed486cc9e78f075ec74f3c9c48eb2a0efcc0c1b
Final worktree: clean after commit
Push status: success; `origin/prob-livo` recorded at the I0 final commit
```

## Prompt 1 / I1 evidence

```text
I1 start HEAD: 9ed486cc9e78f075ec74f3c9c48eb2a0efcc0c1b
Reference SHA: 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
Prompt registration: prompts/prob_livo/prompt1_prob_eskf19.md
Prompt registration SHA256: 7e90c582356715a6d53d73f10b6888ef01f365bb77c19769b427d3af79a2efda
Oracle: tests/prob_livo/oracle/super_eskf_oracle.h, reference SHA above,
        ESKF.cpp Predict 187-249 / UpdateObserve 251-336
```

Focused test command and result:

```bash
source /home/lc/design_ws/devel/setup.bash
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i1_tests
```

Return code: `0`. The runner reports PASS for G-I1.1 through G-I1.8 with
deterministic dense-SPD and negative fixtures. Maximum observed parity errors
were: update physical covariance `6.26e-13`, nonzero-cross full P19 predict
`1.14e-13`, update state `2.93e-15`, and covariance symmetry `9.06e-14`.

Full build command and result:

```bash
source /home/lc/design_ws/devel/setup.bash
catkin_make --pkg fast_livo
```

Return code: `0`; existing `fastlivo_mapping` and host libraries linked, and
the isolated `prob_livo_i1_tests` target linked. No runtime executable was
launched and no rosbag was run.

Scope audit: `include/common_lib.h` and `src/vio.cpp` have no diff from the I1
start; no `LIVMapper` callsite references `ProbESKF19`; no scheduler,
undistortion, OctVox, P1–P4, VIO, or P5 integration was made.

Focused I1 commits before the final evidence close are:

```text
I1 docs/registration commit: 4d1c654
I1 production core commit: 7dfda14
I1 oracle/tests commit: baf6e48
Final evidence-close commit: recorded by `git rev-parse HEAD` after this update
Final worktree: verified clean before push
Push status: fast-forward push to origin/prob-livo follows final verification
```

## Prompt 2 / I1 corrective + I2 evidence

```text
Prompt 2 source: /home/lc/super_livo/prompts/prompt2_i1_close_i2_super_imu.md
Prompt registration: prompts/prob_livo/prompt2_i1_close_i2_super_imu.md
Prompt registration SHA256: cc601777aa8529da675eaa5afcaa7f877d1d12f543f746ee7773c9fa20964888
Prompt 2 start HEAD: ecd8058f08bcae987f3d87934f237964409773bf
I1 corrective SHA: a3458c6 (fix callback lifecycle seam)
I2 implementation/test SHA: 689e0d3
Reference SHA: 9fc949f46291c0fa76e5b7cdb372c940eb4b3f6e
```

Before edits, `G-P2.0` passed at the Owner frontier: the existing
`prob_livo_i1_tests` returned `0` and the overlaid command
`source /home/lc/design_ws/devel/setup.bash && catkin_make --pkg fast_livo`
returned `0` from `/home/lc/super_livo`.

The I1 callback corrective is direct: `UpdateObserve` passes
`need_converge` into the observation callback before it runs. The production
and independent Super-oracle sequences are `[F,F]` and `[F,F,F,T]`; the
existing numerical I1 envelope remains green. Actual Super SO(3) golden
fixtures were generated from `src/basic/src/Manifold.cpp` at the reference
SHA. Maximum observed errors are matrix `1.0541e-7`, log `6.6698e-8`, and
round-trip `5.55e-17`.

### I2 source audit

```text
FAST scheduler: src/LIVMapper.cpp:884-1030
  ONLY_LIO end = lidar_frame_end_time; IMU consume <= end; raw curvature is
  scan-header-relative.
  LIVO end = image_capture_time; IMU consume <= image time; current points
  are rebased to prior last_lio_update_time and next points to image time.
FAST packet/types: include/common_lib.h:62-100
FAST IMU entry: src/LIVMapper.cpp:248-265 (existing Process2 path)
Super init: src/super_lio/src/lio/super_lio.cpp:120-165
Super endpoint/undistort: src/super_lio/src/lio/super_lio.cpp:384-449
Super Predict: src/super_lio/src/lio/ESKF.cpp:187-247
```

The canonical wrapper sets the observation endpoint to `lidar.end_time` and
passes its IMU sequence to `ESKF::Predict`; a sample beyond the endpoint is
used to form the clipped final interval while remaining in IMU history. FAST
does not expose that sample inside the consumed measure, so `ProbImuAdapter`
requires an explicit non-consuming look-ahead only when needed. Missing
coverage is rejected before state mutation.

### I2 focused evidence

```bash
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i1_tests
/home/lc/super_livo/devel/lib/fast_livo/prob_livo_i2_tests
```

Both returned `0`. I2 reports PASS for G-I2.1 through G-I2.9. Initialization
state/covariance and propagated physical state/covariance errors are zero in
the deterministic double-precision oracle fixture. The accepted trace
`(time,R,p,v,a,w)` maximum error is `0`. Maximum undistorted XYZ error across
no-motion, translation, rotation, coupled-motion, endpoint, midpoint, and
nonidentity-extrinsic cases is `2.0415e-7`, within the float point-cloud
storage tolerance.

The LIVO scheduler helper is production-used by `sync_packages`; tests verify
pre/post-camera partition, the two curvature origins, `<= lio_time` IMU
ownership, and source-level helper use. Three consecutive epochs verify
single advancement of `last_lio_update_time`. A source guard confirms no
`ProcessLioEpoch` or `prob_scan_undistort_imu` call is present in
`LIVMapper.cpp`, while the existing `p_imu->Process2` path remains.

Final build:

```bash
source /home/lc/design_ws/devel/setup.bash
catkin_make --pkg fast_livo
```

Return code: `0`. No rosbag or dataset was run. No OctVox/P1–P4 backend, VIO
behavior, camera runtime, or I3 work was started. `include/common_lib.h` and
`src/vio.cpp` remain unchanged from the I2 start. Final worktree cleanliness,
final HEAD, and push result are recorded after the close commit below.
