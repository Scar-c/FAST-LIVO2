# Prob-LIVO Prompt Index

```text
P0 / I0  bootstrap + architecture freeze
P1 / I1  ProbESKF19
P2 / I2  Super IMU + undistort under LIVO2 scheduler
P3 / I3  Prob-LIO P0–P4 backend + camera-OFF baseline
P4 / I3  Super-input parity corrective
P5 / I3  numeric divergence attribution
P6 / I4  pointWithVar adapter + numeric closure
P7 / I5  corrective pointWithVar + ProbPlaneProvider
P8 / I6  camera-ON visual closure + visual-gate ablation
P9 / I6  corrective closure: QR Jacobian, validity split, hot-path/radius cleanup
P10 / I6  dead-code hygiene + native-LIO/Prob-LIO memory characterization
I7       downsample ablation cancelled; Super VoxelGridClosest frozen
I8       generalization
```

Prompt 4 / I3 is the Super-input parity corrective for `eee_01`; it does not
start I4.

Stage semantics remain I0–I8 as defined by `spec/prob_livo/SPEC.md`. Future
prompts may split a production seam, but may not silently change stage
ownership, the one-state contract, or the P0–P4/P5 boundary.

Prompt 0 is complete at bootstrap only. Next stage: I1 `ProbESKF19`.

The exact Prompt 1 text is registered at
`prompts/prob_livo/prompt1_prob_eskf19.md`.

The exact Prompt 2 text is registered at
`prompts/prob_livo/prompt2_i1_close_i2_super_imu.md`.

The exact Prompt 3 text is registered at
`prompts/prob_livo/prompt3_i2_close_i3_prob_lio_baseline.md`.

The exact Prompt 4 text is registered at
`prompts/prob_livo/prompt4_super_input_parity_eee01.md`.

Current boundary: I0–I5 are closed/owner-verified; Prompt 9 closed I6's
camera-on visual path after correcting the QR-native visual Jacobian, splitting
geometry and uncertainty validity, removing repeated query eigensolvers, and
passing the required normal-online/offline exact parity check. Prompt 10 then
closed the bounded dead-helper hygiene pass and the eee_01 native-LIO versus
Prob-LIO ATE/memory characterization. The I7 downsample ablation is cancelled
and Super VoxelGridClosest remains frozen.

The complete Prompt 4 report is in
`spec/prob_livo/PROMPT4_EVIDENCE.md`.

The complete Prompt 7 report is in
`spec/prob_livo/PROMPT7_EVIDENCE.md`; the complete Prompt 8 report is in
`spec/prob_livo/PROMPT8_EVIDENCE.md`. The user-requested offline runner is
the preferred regression path after one H1 online/offline parity check.

The exact Prompt 9 text is registered at
`prompts/prob_livo/prompt9_i6_corrective_closure.md`. Prompt 9 requires one
post-correction comparison of the normal default online ROS path against the
canonical offline runner; deterministic one-callback stepping is not valid for
that comparison.

The exact Prompt 10 text is registered at
`prompts/prob_livo/prompt10_native_lio_memory.md`. Prompt 10 is a bounded I6
hygiene and eee_01 characterization task; it must stop after the N0/H0/H0-LIOONLY
comparison and must not enter LIVO/H1/H2 or I8.

The complete Prompt 10 report is in
`spec/prob_livo/PROMPT10_EVIDENCE.md`; Prompt 10 is
`PROMPT10 CHARACTERIZATION CLOSED` and stops for owner review.
