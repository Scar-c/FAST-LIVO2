# Prob-LIVO Evidence Index

Prompt 0 evidence is source identity, source audit, build output, and file
inventory only. No bag-run or accuracy evidence is claimed.

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
of NTU/OXFORD bags, metadata, sizes, and future stage notes. No bag was run.

## Prompt 0 commit evidence

Filled after the focused bootstrap commit is created:

```text
Prompt 0 commit: this focused bootstrap commit; final SHA is reported with `git rev-parse HEAD`
Final HEAD: reported with `git rev-parse HEAD`
Final worktree: clean after commit
Push status: blocked; network DNS failed and external push approval was rejected
```
