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

Current boundary: I0–I5 are closed/owner-verified; Prompt 8 closes I6 as
`CLOSED/PASS — Owner audit pending` with camera-on visual activity and the
visual-gate ablation complete. The H1 online/offline parity check is exact;
the preferred regression path is now the project-owned offline runner. The
I7 downsample ablation is cancelled and Super VoxelGridClosest remains frozen.

The complete Prompt 4 report is in
`spec/prob_livo/PROMPT4_EVIDENCE.md`.

The complete Prompt 7 report is in
`spec/prob_livo/PROMPT7_EVIDENCE.md`; the complete Prompt 8 report is in
`spec/prob_livo/PROMPT8_EVIDENCE.md`. The user-requested offline runner is
the preferred regression path after one H1 online/offline parity check.
