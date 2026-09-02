# Prob-LIVO Prompt Index

```text
P0 / I0  bootstrap + architecture freeze
P1 / I1  ProbESKF19
P2 / I2  Super IMU + undistort under LIVO2 scheduler
P3 / I3  Prob-LIO P0–P4 backend + camera-OFF baseline
P4 / I4  pointWithVar adapter
P5 / I5  ProbPlaneProvider
P6 / I6  camera-ON visual closure
P7 / I7  visual/downsample ablations
P8 / I8  generalization
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

Current boundary: I0, I1, and I2 are closed/owner-verified; Prompt 4 closes
the I3 input-semantics corrective as `CLOSED/PASS — Owner audit pending` with
classification `SUPER_INPUT_TRAJECTORY_NEAR_PARITY`. The historical
FAST-native `0.05290159739482509 m` result remains separate. Current online and
offline native control both measure `0.054502750 m` and are strict-consistent.
Next stage: I4 `pointWithVar`-compatible current-scan adapter.

The complete Prompt 4 report is in
`spec/prob_livo/PROMPT4_EVIDENCE.md`.
