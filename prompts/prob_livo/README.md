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

Stage semantics remain I0–I8 as defined by `spec/prob_livo/SPEC.md`. Future
prompts may split a production seam, but may not silently change stage
ownership, the one-state contract, or the P0–P4/P5 boundary.

Prompt 0 is complete at bootstrap only. Next stage: I1 `ProbESKF19`.

The exact Prompt 1 text is registered at
`prompts/prob_livo/prompt1_prob_eskf19.md`.

The exact Prompt 2 text is registered at
`prompts/prob_livo/prompt2_i1_close_i2_super_imu.md`.
