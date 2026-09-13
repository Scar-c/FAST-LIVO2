# Canonical 4-way stride/resource matrix

## A. Scope and authority

This is a documentation/data extraction from the accepted Prompt22 FEJ-OFF evidence. It is not a new experiment. The matrix covers 13 sequences × 4 final arms × 3 admitted repetitions per cell: 156 run rows and 52 aggregate cells. No dataset was rerun and no estimator semantic was changed in Prompt39.

Authority: `PROMPT22_RUN_LEDGER.csv`, `PROMPT22_FINAL_MATRIX.csv`, `PROMPT22_EVIDENCE.md`, and `PROMPT22_SHARED_CONFIG_MANIFEST.csv`. Prompt22 authority commit is `94fe832d826e464417ff90693d1f2aafae5b255a`; the accepted cleanup anchor before this docs follow-up is `c0921ed2884eae561eaebeb3596f22d327fc1418`.

Arm definitions: `N` = Native FAST-LIVO2; `P` = canonical Prob-LIO P0–P4; `LIO` = camera inactive; `LIVO-STRIDE` = camera active under the frozen camera-selection policy. “Stride” is a camera-selection property only and does not change LIO rows. For LIO, the effective camera stride is `N/A (camera off)`.

## B. Final accuracy table

ATE values are Prompt22 final-cell mean ± population standard deviation, in meters. The last column is the effective LIVO camera stride; LIO is camera-off.

### NTU

| Sequence | N-LIO ATE | P-LIO ATE | N-LIVO-STRIDE ATE | P-LIVO-STRIDE ATE | LIVO stride |
|---|---:|---:|---:|---:|---:|
| eee_01 | 0.030760 ± 0.000000 m | 0.049511 ± 0.000000 m | 0.030000 ± 0.001716 m | 0.052103 ± 0.000099 m | 1 |
| eee_02 | 0.021901 ± 0.000000 m | 0.017281 ± 0.000000 m | 0.035967 ± 0.000085 m | 0.027541 ± 0.000167 m | 1 |
| eee_03 | 0.031572 ± 0.000000 m | 0.029984 ± 0.000000 m | 0.030214 ± 0.000025 m | 0.029665 ± 0.000000 m | 1 |
| nya_01 | 0.027764 ± 0.000000 m | 0.023256 ± 0.000000 m | 0.037922 ± 0.000060 m | 0.038840 ± 0.000008 m | 1 |
| nya_02 | 0.029234 ± 0.000000 m | 0.029557 ± 0.000000 m | 0.036970 ± 0.000001 m | 0.046926 ± 0.000052 m | 1 |
| nya_03 | 0.028883 ± 0.000000 m | 0.025757 ± 0.000000 m | 0.033337 ± 0.000007 m | 0.031924 ± 0.000021 m | 1 |
| sbs_01 | 0.027766 ± 0.000000 m | 0.025729 ± 0.000000 m | 0.028969 ± 0.000046 m | 0.029509 ± 0.000000 m | 1 |
| sbs_02 | 0.026968 ± 0.000000 m | 0.024137 ± 0.000000 m | 0.028706 ± 0.000018 m | 0.028534 ± 0.000007 m | 1 |
| sbs_03 | 0.033383 ± 0.000000 m | 0.025554 ± 0.000000 m | 0.031164 ± 0.000012 m | 0.028208 ± 0.000028 m | 1 |

### Oxford

| Sequence | N-LIO ATE | P-LIO ATE | N-LIVO-STRIDE ATE | P-LIVO-STRIDE ATE | LIVO stride |
|---|---:|---:|---:|---:|---:|
| Church_05 | 0.167800 ± 0.000000 m | 0.244000 ± 0.000000 m | 0.142933 ± 0.002446 m | 0.229933 ± 0.003682 m | 1 |
| College_03 | 0.048100 ± 0.000000 m | 0.074800 ± 0.000000 m | 0.070933 ± 0.002819 m | 0.182500 ± 0.127776 m | 2 |
| Palace_01 | 0.079500 ± 0.000000 m | 0.127200 ± 0.000000 m | 0.103133 ± 0.000896 m | 0.130767 ± 0.000249 m | 2 |
| Quarter_01 | 0.049500 ± 0.000000 m | 0.051000 ± 0.000000 m | 0.047200 ± 0.000294 m | 0.064933 ± 0.001452 m | 2 |

## C. Resource table

Medians are shown for owner readability; the CSV retains mean, population SD, and median for every required resource field. Memory values are MiB. ATE is mean ± SD.

| Sequence | Arm | ATE mean±SD | Est wall median s | Offline wall median s | Peak RSS median MiB | Peak PSS median MiB | Peak USS median MiB |
|---|---|---:|---:|---:|---:|---:|---:|
| NTU/eee_01 | N-LIO | 0.030760 ± 0.000000 m | 47.423448 | 51.533100 | 2587.050781 | 2566.581055 | 2563.773438 |
| NTU/eee_01 | P-LIO | 0.049511 ± 0.000000 m | 37.123917 | 41.465800 | 339.773438 | 319.610352 | 316.703125 |
| NTU/eee_01 | N-LIVO-STRIDE | 0.030000 ± 0.001716 m | 81.304779 | 86.995000 | 4427.027344 | 4406.758789 | 4404.777344 |
| NTU/eee_01 | P-LIVO-STRIDE | 0.052103 ± 0.000099 m | 73.448305 | 78.236400 | 2107.324219 | 2086.849609 | 2084.074219 |
| NTU/eee_02 | N-LIO | 0.021901 ± 0.000000 m | 34.840128 | 38.073400 | 1715.765625 | 1696.568359 | 1693.574219 |
| NTU/eee_02 | P-LIO | 0.017281 ± 0.000000 m | 28.597297 | 32.246700 | 294.585938 | 275.262695 | 273.148438 |
| NTU/eee_02 | N-LIVO-STRIDE | 0.035967 ± 0.000085 m | 66.620611 | 71.176000 | 3110.363281 | 3090.549805 | 3087.917969 |
| NTU/eee_02 | P-LIVO-STRIDE | 0.027541 ± 0.000167 m | 57.269687 | 61.174000 | 1711.257812 | 1690.840820 | 1688.800781 |
| NTU/eee_03 | N-LIO | 0.031572 ± 0.000000 m | 20.742828 | 23.004200 | 1331.226562 | 1311.625977 | 1308.671875 |
| NTU/eee_03 | P-LIO | 0.029984 ± 0.000000 m | 16.369150 | 18.547300 | 249.234375 | 229.493164 | 226.527344 |
| NTU/eee_03 | N-LIVO-STRIDE | 0.030214 ± 0.000025 m | 36.571507 | 39.213400 | 2221.691406 | 2203.934570 | 2202.484375 |
| NTU/eee_03 | P-LIVO-STRIDE | 0.029665 ± 0.000000 m | 33.398752 | 35.702000 | 1065.882812 | 1046.297852 | 1043.312500 |
| NTU/nya_01 | N-LIO | 0.027764 ± 0.000000 m | 36.397002 | 40.617400 | 577.152344 | 557.533203 | 554.500000 |
| NTU/nya_01 | P-LIO | 0.023256 ± 0.000000 m | 26.085412 | 31.160100 | 266.238281 | 246.493164 | 243.507812 |
| NTU/nya_01 | N-LIVO-STRIDE | 0.037922 ± 0.000060 m | 78.868458 | 88.212200 | 2307.460938 | 2286.850586 | 2284.765625 |
| NTU/nya_01 | P-LIVO-STRIDE | 0.038840 ± 0.000008 m | 59.657260 | 64.556200 | 2043.824219 | 2023.454102 | 2020.765625 |
| NTU/nya_02 | N-LIO | 0.029234 ± 0.000000 m | 41.405045 | 45.729400 | 562.164062 | 542.759766 | 540.730469 |
| NTU/nya_02 | P-LIO | 0.029557 ± 0.000000 m | 29.592655 | 35.084400 | 279.046875 | 259.196289 | 256.023438 |
| NTU/nya_02 | N-LIVO-STRIDE | 0.036970 ± 0.000001 m | 82.772658 | 90.348100 | 2432.148438 | 2411.651367 | 2408.964844 |
| NTU/nya_02 | P-LIVO-STRIDE | 0.046926 ± 0.000052 m | 65.724684 | 71.101900 | 2186.753906 | 2167.861328 | 2164.808594 |
| NTU/nya_03 | N-LIO | 0.028883 ± 0.000000 m | 38.822069 | 42.944100 | 647.683594 | 627.423828 | 624.605469 |
| NTU/nya_03 | P-LIO | 0.025757 ± 0.000000 m | 27.954662 | 32.865300 | 291.796875 | 272.669922 | 269.589844 |
| NTU/nya_03 | N-LIVO-STRIDE | 0.033337 ± 0.000007 m | 76.406656 | 83.336400 | 2337.835938 | 2318.840820 | 2315.765625 |
| NTU/nya_03 | P-LIVO-STRIDE | 0.031924 ± 0.000021 m | 62.920539 | 68.587700 | 2124.511719 | 2105.049805 | 2102.121094 |
| NTU/sbs_01 | N-LIO | 0.027766 ± 0.000000 m | 37.801766 | 42.028900 | 1847.929688 | 1827.906250 | 1825.121094 |
| NTU/sbs_01 | P-LIO | 0.025729 ± 0.000000 m | 24.892286 | 28.861100 | 303.101562 | 282.477539 | 279.570312 |
| NTU/sbs_01 | N-LIVO-STRIDE | 0.028969 ± 0.000046 m | 63.645416 | 69.087200 | 3397.160156 | 3378.844727 | 3377.339844 |
| NTU/sbs_01 | P-LIVO-STRIDE | 0.029509 ± 0.000000 m | 53.955573 | 58.388800 | 1882.101562 | 1864.741211 | 1863.281250 |
| NTU/sbs_02 | N-LIO | 0.026968 ± 0.000000 m | 38.306300 | 42.474300 | 1806.519531 | 1787.184570 | 1784.445312 |
| NTU/sbs_02 | P-LIO | 0.024137 ± 0.000000 m | 25.855050 | 30.368100 | 303.417969 | 284.233398 | 281.054688 |
| NTU/sbs_02 | N-LIVO-STRIDE | 0.028706 ± 0.000018 m | 73.302783 | 78.995000 | 3496.636719 | 3477.099609 | 3474.167969 |
| NTU/sbs_02 | P-LIVO-STRIDE | 0.028534 ± 0.000007 m | 58.481360 | 63.081800 | 1997.039062 | 1977.630859 | 1974.503906 |
| NTU/sbs_03 | N-LIO | 0.033383 ± 0.000000 m | 30.436598 | 33.944900 | 1806.437500 | 1788.224609 | 1785.324219 |
| NTU/sbs_03 | P-LIO | 0.025554 ± 0.000000 m | 22.323879 | 25.893400 | 326.480469 | 306.847656 | 303.839844 |
| NTU/sbs_03 | N-LIVO-STRIDE | 0.031164 ± 0.000012 m | 71.714419 | 78.285300 | 3531.839844 | 3511.895508 | 3508.933594 |
| NTU/sbs_03 | P-LIVO-STRIDE | 0.028208 ± 0.000028 m | 50.166480 | 54.295300 | 2076.203125 | 2056.936523 | 2053.980469 |
| OXFORD/Church_05 | N-LIO | 0.167800 ± 0.000000 m | 111.583076 | 127.445000 | 995.269531 | 975.184570 | 971.902344 |
| OXFORD/Church_05 | P-LIO | 0.244000 ± 0.000000 m | 107.440542 | 124.902000 | 823.500000 | 803.577148 | 800.324219 |
| OXFORD/Church_05 | N-LIVO-STRIDE | 0.142933 ± 0.002446 m | 126.060768 | 183.004000 | 8059.960938 | 8040.157227 | 8036.878906 |
| OXFORD/Church_05 | P-LIVO-STRIDE | 0.229933 ± 0.003682 m | 241.765378 | 300.753000 | 8096.460938 | 8076.423828 | 8075.046875 |
| OXFORD/College_03 | N-LIO | 0.048100 ± 0.000000 m | 55.265122 | 69.171800 | 597.835938 | 577.867188 | 574.742188 |
| OXFORD/College_03 | P-LIO | 0.074800 ± 0.000000 m | 51.368614 | 62.927700 | 647.250000 | 627.128906 | 623.917969 |
| OXFORD/College_03 | N-LIVO-STRIDE | 0.070933 ± 0.002819 m | 83.415357 | 124.472000 | 5915.804688 | 5896.025391 | 5892.812500 |
| OXFORD/College_03 | P-LIVO-STRIDE | 0.182500 ± 0.127776 m | 125.153799 | 167.541000 | 5967.792969 | 5948.583008 | 5945.328125 |
| OXFORD/Palace_01 | N-LIO | 0.079500 ± 0.000000 m | 70.382259 | 85.448700 | 953.425781 | 933.408203 | 930.144531 |
| OXFORD/Palace_01 | P-LIO | 0.127200 ± 0.000000 m | 64.875687 | 79.230700 | 671.179688 | 651.172852 | 648.000000 |
| OXFORD/Palace_01 | N-LIVO-STRIDE | 0.103133 ± 0.000896 m | 106.325946 | 162.989000 | 8042.722656 | 8022.922852 | 8019.726562 |
| OXFORD/Palace_01 | P-LIVO-STRIDE | 0.130767 ± 0.000249 m | 182.621960 | 240.739000 | 7890.351562 | 7870.300781 | 7867.101562 |
| OXFORD/Quarter_01 | N-LIO | 0.049500 ± 0.000000 m | 65.813070 | 79.837700 | 1065.167969 | 1044.832031 | 1041.656250 |
| OXFORD/Quarter_01 | P-LIO | 0.051000 ± 0.000000 m | 59.389697 | 71.500700 | 541.019531 | 521.189453 | 518.015625 |
| OXFORD/Quarter_01 | N-LIVO-STRIDE | 0.047200 ± 0.000294 m | 92.988067 | 134.762000 | 5998.550781 | 5979.124023 | 5975.859375 |
| OXFORD/Quarter_01 | P-LIVO-STRIDE | 0.064933 ± 0.001452 m | 130.895992 | 174.208000 | 6093.300781 | 6074.316406 | 6071.085938 |

## D. Visual workload and stride

| Sequence | LIO effective stride | N-LIVO-STRIDE (stride; visual calls; visual commits) | P-LIVO-STRIDE (stride; visual calls; visual commits) |
|---|---|---|---|
| NTU/eee_01 | N/A (camera off) | 1; 3983;3983;3983; 3982;3982;3982 | 1; 3979;3979;3979; 3977;3977;3977 |
| NTU/eee_02 | N/A (camera off) | 1; 3207;3207;3207; 3205;3205;3205 | 1; 3202;3202;3202; 3200;3200;3200 |
| NTU/eee_03 | N/A (camera off) | 1; 1811;1811;1811; 1809;1809;1809 | 1; 1806;1806;1806; 1804;1804;1804 |
| NTU/nya_01 | N/A (camera off) | 1; 3946;3946;3946; 3944;3944;3944 | 1; 3941;3941;3941; 3939;3939;3939 |
| NTU/nya_02 | N/A (camera off) | 1; 4283;4283;4283; 4282;4282;4282 | 1; 4279;4279;4279; 4277;4277;4277 |
| NTU/nya_03 | N/A (camera off) | 1; 4091;4091;4091; 4089;4089;4089 | 1; 4086;4086;4086; 4084;4084;4084 |
| NTU/sbs_01 | N/A (camera off) | 1; 3538;3538;3538; 3537;3537;3537 | 1; 3534;3534;3534; 3532;3532;3532 |
| NTU/sbs_02 | N/A (camera off) | 1; 3728;3728;3728; 3727;3727;3727 | 1; 3724;3724;3724; 3722;3722;3722 |
| NTU/sbs_03 | N/A (camera off) | 1; 3890;3890;3890; 3888;3888;3888 | 1; 3885;3885;3885; 3883;3883;3883 |
| OXFORD/Church_05 | N/A (camera off) | 1; 3960;3960;3960; 3959;3959;3959 | 1; 3956;3956;3956; 3953;3954;3954 |
| OXFORD/College_03 | N/A (camera off) | 2; 2839;2839;2839; 2837;2837;2837 | 2; 2834;2834;2834; 2832;2831;2831 |
| OXFORD/Palace_01 | N/A (camera off) | 2; 4019;4019;4019; 4017;4017;4017 | 2; 4014;4014;4014; 4012;4012;4011 |
| OXFORD/Quarter_01 | N/A (camera off) | 2; 2871;2871;2871; 2869;2869;2869 | 2; 2866;2866;2866; 2864;2864;2864 |

The semicolon-separated workload values preserve the three Prompt22 repetition values. LIO visual calls and commits remain `0;0;0` according to the source matrix.

## E. Native versus Prob summary

All comparisons below use only this Prompt22-derived matrix. Negative ATE delta means Prob is lower (better) than Native; positive means higher (worse).

| Comparison | Prob lower ATE | Prob higher ATE |
|---|---|---|
| P-LIO vs N-LIO | NTU/eee_02, NTU/eee_03, NTU/nya_01, NTU/nya_03, NTU/sbs_01, NTU/sbs_02, NTU/sbs_03 | NTU/eee_01, NTU/nya_02, OXFORD/Church_05, OXFORD/College_03, OXFORD/Palace_01, OXFORD/Quarter_01 |
| P-LIVO-STRIDE vs N-LIVO-STRIDE | NTU/eee_02, NTU/eee_03, NTU/nya_03, NTU/sbs_02, NTU/sbs_03 | NTU/eee_01, NTU/nya_01, NTU/nya_02, NTU/sbs_01, OXFORD/Church_05, OXFORD/College_03, OXFORD/Palace_01, OXFORD/Quarter_01 |

Resource ratios below are Prob / Native medians across the 13 sequence-level cells; `<1` means the Prob median was lower for that resource. They are descriptive sequence-level summaries, not a tuning recommendation.

| Comparison | Metric | Median ratio | Min | Max |
|---|---|---:|---:|---:|
| P-LIO / N-LIO | Estimator wall | 0.783 | 0.658 | 0.963 |
| P-LIO / N-LIO | Offline wall | 0.805 | 0.687 | 0.980 |
| P-LIO / N-LIO | Peak RSS | 0.451 | 0.131 | 1.083 |
| P-LIO / N-LIO | Peak PSS | 0.435 | 0.125 | 1.085 |
| P-LIO / N-LIO | Peak USS | 0.432 | 0.124 | 1.086 |
| P-LIVO-STRIDE / N-LIVO-STRIDE | Estimator wall | 0.860 | 0.700 | 1.918 |
| P-LIVO-STRIDE / N-LIVO-STRIDE | Offline wall | 0.859 | 0.694 | 1.643 |
| P-LIVO-STRIDE / N-LIVO-STRIDE | Peak RSS | 0.886 | 0.476 | 1.016 |
| P-LIVO-STRIDE / N-LIVO-STRIDE | Peak PSS | 0.885 | 0.474 | 1.016 |
| P-LIVO-STRIDE / N-LIVO-STRIDE | Peak USS | 0.884 | 0.473 | 1.016 |

## F. Provenance note

Prompt19 was the historical Oxford stride/resource-development round. Prompt22 corrected the Oxford Native/Prob shared configuration and re-controlled the final 4-way matrix; therefore this standalone matrix uses Prompt22, not old Prompt19 Oxford Native cells.

FEJ production, common-linearization/CN, and SA-CN/SA-LIVO are not represented in these rows. Prompt23 remains ablation-only. Preserved FEJ-OFF controls and historical archives are documented by the canonical cleanup files.
