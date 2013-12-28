#ifndef Array_H
#define Array_H
/*
[FIX 86,473 REP 19,362]sequence: 105,835
[NUC 20,268,888 UNK 236,387 OP 34,108,867 LEN 97,167,150 LOC 6,430,732 STI 2,463]editOp: 158,214,487
[IDX 13,045,284 S 185,178,151]readName: 198,223,435
mapFlag: 16,547,932
mapQual: 20,476,571
quality: 1,029,975,112
pairedEnd: 150,710,716
optField: 157,380,812
kamata 3.17

SC QP DZ_1
24 ** 32

samtools
real    12m40.974s
user    11m56.281s
sys     0m14.746s

quip
real    5m16.912s
user    7m49.969s
sys     0m7.865s

sam_comp
real    6m37.028s
user    6m10.649s
sys     0m8.442s

dz
real    5m45.333s
user    9m48.500s
sys     0m18.812s


-rw-rw-r--. 1 inumanag inumanag  2833376718 Dec 23 20:35 ERX009603.bam
-rw-rw-r--. 1 inumanag inumanag  1737046074 Dec 23 21:18 ERX009603.dz
1661787739
                                 1428954546
-rw-rw-r--. 1 inumanag inumanag  1625981137 Dec 23 20:58 ERX009603.quip
-rw-rw-r--. 1 inumanag inumanag 12238463169 Dec 23 20:52 ERX009603.samqu
-rw-rw-r--. 1 inumanag inumanag  1228493655 Dec 23 21:05 ERX009603.zam


# [REQ] 2.34 [S1] 0.19
[T~] 0.39 [T~] 0.46 [T~] 0.51 [T~] 0.62 [S2] 0.62
[S3] 0.01 [FIX] 0.83 [SQ] 0.00 [MQ] 0.36 [MF] 0.48 [PE] 1.07 [OF] 1.54 [RN] 1.76 [EO] 1.95 [QQ] 3.47 [CALC] 3.47 [IO] 0.04
   Chr chr22_M33388  4.74% [2]
# [REQ] 2.31 [S1] 0.00
[T~] 0.12 [T~] 0.17 [T~] 0.71 [T~] 0.85 [S2] 0.85
[S3] 0.00 [FIX] 0.87 [SQ] 0.02 [MQ] 0.29 [MF] 0.57 [PE] 1.10 [OF] 1.44 [RN] 1.53 [EO] 2.22 [QQ] 2.89 [CALC] 2.89 [IO] 0.04
   Chr chr22_M33388  6.86% [3]
# [REQ] 2.08 [S1] 0.01
[T~] 0.17 [T~] 0.19 [T~] 0.64 [T~] 1.02 [S2] 1.02
[S3] 0.01 [FIX] 1.05 [SQ] 0.00 [MF] 0.30 [MQ] 0.32 [OF] 1.36 [PE] 1.38 [RN] 1.52 [EO] 2.41 [QQ] 3.42 [CALC] 3.43 [IO] 0.05
   Chr chr22_M33388  9.18% [4]
# [REQ] 2.36 [S1] 0.01
[T~] 0.06 [T~] 0.25 [T~] 0.74 [T~] 0.96 [S2] 0.96
[S3] 0.00 [FIX] 0.98 [SQ] 0.00 [MQ] 0.48 [MF] 0.82 [PE] 0.93 [OF] 1.81 [RN] 1.94 [EO] 2.35 [QQ] 3.13 [CALC] 3.13 [IO] 0.05
   Chr chr22_M33388 11.59% [5]
# [REQ] 2.46 [S1] 0.00
[T~] 0.16 [T~] 0.36 [T~] 0.49 [T~] 1.03 [S2] 1.03
[S3] 0.00 [FIX] 1.05 [SQ] 0.00 [MQ] 0.17 [MF] 0.55 [PE] 1.16 [RN] 1.48 [OF] 1.53 [EO] 2.24 [QQ] 3.48 [CALC] 3.49 [IO] 0.05
   Chr chr22_M33388 13.90% [6]
# [REQ] 2.56 [S1] 0.01
[T~] 0.12 [T~] 0.26 [T~] 0.73 [T~] 0.97 [S2] 0.97
[S3] 0.01 [FIX] 0.99 [SQ] 0.00 [MQ] 0.29 [MF] 0.60 [PE] 0.94 [OF] 1.58 [RN] 1.89 [EO] 2.22 [QQ] 3.52 [CALC] 3.52 [IO] 0.05
   Chr chr22_M33388 16.30% [7]
# [REQ] 2.75 [S1] 0.01
[T~] 0.12 [T~] 0.14 [T~] 0.72 [T~] 1.12 [S2] 1.12
[S3] 0.00 [FIX] 1.14 [SQ] 0.00 [MQ] 0.15 [MF] 0.43 [PE] 1.17 [OF] 1.67 [RN] 1.82 [EO] 2.32 [QQ] 3.55 [CALC] 3.55 [IO] 0.05
   Chr chr22_M33388 18.70% [8]
# [REQ] 2.84 [S1] 0.01
[T~] 0.11 [T~] 0.22 [T~] 0.61 [T~] 1.07 [S2] 1.07
[S3] 0.01 [FIX] 1.10 [SQ] 0.00 [MQ] 0.20 [MF] 0.56 [PE] 1.18 [OF] 1.74 [RN] 1.91 [EO] 2.24 [QQ] 3.77 [CALC] 3.78 [IO] 0.05
   Chr chr22_M33388 21.10% [9]
# [REQ] 2.83 [S1] 0.03
[T~] 0.35 [T~] 0.50 [T~] 0.63 [T~] 0.72 [S2] 0.72
[S3] 0.03 [FIX] 0.80 [SQ] 0.00 [MQ] 0.15 [MF] 0.39 [PE] 1.20 [RN] 1.67 [OF] 1.88 [EO] 2.39 [QQ] 3.80 [CALC] 3.82 [IO] 0.05
   Chr chr22_M33388 23.47% [10]
# [REQ] 2.96 [S1] 0.05
[T~] 0.42 [T~] 0.45 [T~] 0.59 [T~] 0.78 [S2] 0.79
[S3] 0.05 [FIX] 0.90 [SQ] 0.00 [MQ] 0.18 [MF] 0.62 [OF] 1.15 [PE] 1.51 [EO] 1.62 [RN] 1.87 [QQ] 4.02 [CALC] 4.03 [IO] 0.05
   Chr chr22_M33388 25.84% [11]
# [REQ] 3.10 [S1] 0.08
[T~] 0.36 [T~] 0.56 [T~] 0.64 [T~] 0.73 [S2] 0.78
[S3] 0.07 [FIX] 0.94 [SQ] 0.01 [MQ] 0.19 [MF] 0.47 [OF] 1.13 [PE] 1.23 [EO] 1.94 [RN] 2.00 [QQ] 3.80 [CALC] 3.81 [IO] 0.05
   Chr chr22_M33388 28.26% [12]
# [REQ] 3.07 [S1] 0.01
[T~] 0.08 [T~] 0.33 [T~] 0.47 [T~] 1.20 [S2] 1.20
[S3] 0.01 [FIX] 1.22 [SQ] 0.00 [MQ] 0.13 [MF] 0.64 [PE] 1.03 [RN] 1.63 [OF] 1.82 [EO] 2.54 [QQ] 3.69 [CALC] 3.69 [IO] 0.05
   Chr chr22_M33388 30.65% [13]
# [REQ] 3.02 [S1] 0.01
[T~] 0.18 [T~] 0.19 [T~] 0.25 [T~] 1.66 [S2] 1.66
[S3] 0.01 [FIX] 1.69 [SQ] 0.00 [MQ] 0.31 [MF] 0.60 [PE] 1.11 [OF] 1.65 [RN] 2.03 [EO] 2.08 [QQ] 3.82 [CALC] 3.82 [IO] 0.05
   Chr chr22_M33388 33.04% [14]
# [REQ] 3.14 [S1] 0.03
[T~] 0.23 [T~] 0.32 [T~] 0.55 [T~] 1.09 [S2] 1.09
[S3] 0.04 [FIX] 1.17 [SQ] 0.01 [MQ] 0.15 [MF] 0.60 [PE] 1.32 [OF] 1.47 [RN] 1.77 [EO] 1.98 [QQ] 4.06 [CALC] 4.07 [IO] 0.05
   Chr chr22_M33388 35.42% [15]
# [REQ] 3.06 [S1] 0.02
[T~] 0.23 [T~] 0.66 [T~] 0.66 [T~] 0.68 [S2] 0.68
[S3] 0.03 [FIX] 0.74 [SQ] 0.00 [MQ] 0.14 [MF] 0.55 [PE] 1.22 [OF] 1.57 [RN] 1.96 [EO] 1.98 [QQ] 3.85 [CALC] 3.86 [IO] 0.05
   Chr chr22_M33388 37.79% [16]
# [REQ] 3.26 [S1] 0.04
[T~] 0.36 [T~] 0.38 [T~] 0.47 [T~] 0.99 [S2] 0.99
[S3] 0.05 [FIX] 1.09 [SQ] 0.00 [MQ] 0.24 [MF] 0.48 [PE] 1.12 [OF] 1.41 [EO] 1.75 [RN] 1.82 [QQ] 4.04 [CALC] 4.06 [IO] 0.05
   Chr chr22_M33388 40.16% [17]
# [REQ] 3.64 [S1] 0.04
[T~] 0.48 [T~] 0.52 [T~] 0.60 [T~] 0.67 [S2] 0.67
[S3] 0.05 [FIX] 0.78 [SQ] 0.01 [MQ] 0.16 [MF] 0.62 [PE] 1.29 [OF] 1.45 [EO] 1.75 [RN] 1.85 [QQ] 3.99 [CALC] 3.99 [IO] 0.05
   Chr chr22_M33388 42.57% [18]
# [REQ] 3.16 [S1] 0.01
[T~] 0.13 [T~] 0.15 [T~] 0.27 [T~] 1.31 [S2] 1.31
[S3] 0.02 [FIX] 1.36 [SQ] 0.00 [MQ] 0.11 [MF] 0.45 [PE] 1.10 [OF] 1.41 [RN] 1.70 [EO] 1.99 [QQ] 3.37 [CALC] 3.38 [IO] 0.05
   Chr chr22_M33388 44.69% [19]
# [REQ] 2.81 [S1] 0.01
[T~] 0.15 [T~] 0.45 [T~] 0.52 [T~] 1.14 [S2] 1.14
[S3] 0.01 [FIX] 1.16 [SQ] 0.00 [MQ] 0.21 [MF] 0.61 [PE] 0.92 [OF] 1.86 [RN] 1.85 [EO] 2.52 [QQ] 3.63 [CALC] 3.63 [IO] 0.05
   Chr chr22_M33388 47.10% [20]
# [REQ] 3.11 [S1] 0.02
[T~] 0.12 [T~] 0.37 [T~] 0.63 [T~] 1.01 [S2] 1.01
[S3] 0.01 [FIX] 1.05 [SQ] 0.00 [MQ] 0.11 [MF] 0.51 [OF] 1.44 [PE] 1.55 [RN] 2.10 [EO] 2.34 [QQ] 3.78 [CALC] 3.80 [IO] 0.05
   Chr chr22_M33388 49.47% [21]
# [REQ] 3.83 [S1] 0.03
[T~] 0.18 [T~] 0.23 [T~] 0.23 [T~] 1.51 [S2] 1.51
[S3] 0.01 [FIX] 1.56 [SQ] 0.01 [MQ] 0.27 [MF] 0.56 [OF] 1.45 [PE] 1.51 [RN] 1.62 [EO] 2.12 [QQ] 3.71 [CALC] 3.71 [IO] 0.05
   Chr chr22_M33388 51.74% [22]
# [REQ] 3.19 [S1] 0.08
[T~] 0.52 [T~] 0.56 [T~] 0.57 [T~] 0.62 [S2] 0.63
[S3] 0.07 [FIX] 0.79 [SQ] 0.02 [MQ] 0.34 [MF] 0.62 [OF] 0.96 [PE] 1.32 [RN] 1.72 [EO] 1.83 [QQ] 3.80 [CALC] 3.81 [IO] 0.05
   Chr chr22_M33388 54.07% [23]
# [REQ] 3.27 [S1] 0.06
[T~] 0.37 [T~] 0.40 [T~] 0.62 [T~] 0.81 [S2] 0.81
[S3] 0.05 [FIX] 0.93 [SQ] 0.01 [MQ] 0.31 [MF] 0.43 [OF] 1.00 [PE] 1.33 [RN] 1.80 [EO] 1.88 [QQ] 3.68 [CALC] 3.68 [IO] 0.05
   Chr chr22_M33388 56.42% [24]
# [REQ] 3.27 [S1] 0.01
[T~] 0.18 [T~] 0.24 [T~] 0.60 [T~] 1.26 [S2] 1.26
[S3] 0.01 [FIX] 1.30 [SQ] 0.00 [MQ] 0.16 [MF] 0.59 [OF] 1.29 [PE] 1.45 [RN] 1.78 [EO] 2.35 [QQ] 4.08 [CALC] 4.08 [IO] 0.05
   Chr chr22_M33388 58.82% [25]
# [REQ] 5.17 [S1] 0.03
[T~] 0.26 [T~] 0.37 [T~] 0.47 [T~] 1.14 [S2] 1.14
[S3] 0.04 [FIX] 1.21 [SQ] 0.00 [MQ] 0.15 [MF] 0.56 [PE] 1.31 [OF] 1.71 [RN] 1.83 [EO] 2.28 [QQ] 3.77 [CALC] 3.78 [IO] 0.05
   Chr chr22_M33388 61.21% [26]
# [REQ] 3.43 [S1] 0.04
[T~] 0.22 [T~] 0.22 [T~] 0.23 [T~] 1.37 [S2] 1.38
[S3] 0.03 [FIX] 1.45 [SQ] 0.00 [MQ] 0.14 [MF] 0.63 [PE] 1.42 [OF] 1.49 [RN] 1.64 [EO] 1.87 [QQ] 4.02 [CALC] 4.02 [IO] 0.05
   Chr chr22_M33388 63.61% [27]
# [REQ] 3.23 [S1] 0.01
[T~] 0.25 [T~] 0.33 [T~] 0.45 [T~] 1.08 [S2] 1.08
[S3] 0.01 [FIX] 1.11 [SQ] 0.00 [MQ] 0.12 [MF] 0.57 [PE] 1.40 [OF] 1.51 [RN] 1.86 [EO] 2.57 [QQ] 3.83 [CALC] 3.83 [IO] 0.05
   Chr chr22_M33388 66.00% [28]
# [REQ] 3.30 [S1] 0.02
[T~] 0.21 [T~] 0.22 [T~] 0.67 [T~] 1.17 [S2] 1.17
[S3] 0.01 [FIX] 1.20 [SQ] 0.00 [MQ] 0.12 [MF] 0.38 [PE] 1.47 [OF] 1.75 [RN] 1.99 [EO] 2.54 [QQ] 3.74 [CALC] 3.75 [IO] 0.05
   Chr chr22_M33388 68.39% [29]
# [REQ] 3.33 [S1] 0.03
[T~] 0.27 [T~] 0.36 [T~] 0.78 [T~] 0.88 [S2] 0.88
[S3] 0.03 [FIX] 0.96 [SQ] 0.00 [MQ] 0.29 [MF] 0.64 [PE] 1.51 [OF] 1.60 [RN] 1.95 [EO] 2.37 [QQ] 3.76 [CALC] 3.77 [IO] 0.05
   Chr chr22_M33388 70.76% [30]
# [REQ] 3.20 [S1] 0.08
[T~] 0.45 [T~] 0.45 [T~] 0.45 [T~] 0.99 [S2] 0.99
[S3] 0.06 [FIX] 1.14 [SQ] 0.01 [MQ] 0.24 [MF] 0.50 [OF] 1.37 [PE] 1.38 [EO] 1.75 [RN] 2.04 [QQ] 4.00 [CALC] 4.01 [IO] 0.05
   Chr chr22_M33388 73.16% [31]
# [REQ] 3.40 [S1] 0.04
[T~] 0.19 [T~] 0.22 [T~] 0.23 [T~] 1.42 [S2] 1.42
[S3] 0.03 [FIX] 1.51 [SQ] 0.00 [MQ] 0.18 [MF] 0.63 [PE] 1.17 [OF] 1.46 [RN] 1.64 [EO] 2.29 [QQ] 4.00 [CALC] 4.00 [IO] 0.05
   Chr chr22_M33388 75.55% [32]
# [REQ] 4.20 [S1] 0.05
[T~] 0.30 [T~] 0.34 [T~] 0.63 [T~] 0.89 [S2] 0.89
[S3] 0.04 [FIX] 1.00 [SQ] 0.00 [MQ] 0.31 [MF] 0.32 [OF] 1.45 [PE] 1.51 [RN] 1.85 [EO] 1.93 [QQ] 3.75 [CALC] 3.76 [IO] 0.05
   Chr chr22_M33388 77.91% [33]
# [REQ] 3.19 [S1] 0.04
[T~] 0.32 [T~] 0.38 [T~] 0.40 [T~] 1.22 [S2] 1.22
[S3] 0.03 [FIX] 1.31 [SQ] 0.00 [MQ] 0.17 [MF] 0.54 [PE] 1.04 [OF] 1.50 [RN] 1.80 [EO] 2.22 [QQ] 3.89 [CALC] 3.89 [IO] 0.05
   Chr chr22_M33388 80.27% [34]
# [REQ] 3.34 [S1] 0.09
[T~] 0.54 [T~] 0.57 [T~] 0.58 [T~] 0.63 [S2] 0.63
[S3] 0.07 [FIX] 0.81 [SQ] 0.01 [MQ] 0.19 [MF] 0.58 [PE] 1.02 [OF] 1.03 [EO] 1.85 [RN] 2.01 [QQ] 3.98 [CALC] 3.98 [IO] 0.05
   Chr chr22_M33388 82.62% [35]
# [REQ] 3.19 [S1] 0.09
[T~] 0.55 [T~] 0.57 [T~] 0.61 [T~] 0.62 [S2] 0.67
[S3] 0.08 [FIX] 0.85 [SQ] 0.01 [MQ] 0.27 [MF] 0.54 [OF] 1.01 [PE] 1.14 [EO] 1.81 [RN] 1.88 [QQ] 3.94 [CALC] 3.95 [IO] 0.05
   Chr chr22_M33388 84.99% [36]
# [REQ] 5.35 [S1] 0.06
[T~] 0.42 [T~] 0.57 [T~] 0.60 [T~] 0.68 [S2] 0.68
[S3] 0.07 [FIX] 0.82 [SQ] 0.02 [MQ] 0.17 [MF] 0.61 [PE] 1.26 [OF] 1.29 [RN] 1.60 [EO] 1.94 [QQ] 3.90 [CALC] 3.91 [IO] 0.05
   Chr chr22_M33388 87.34% [37]
# [REQ] 3.28 [S1] 0.02
[T~] 0.22 [T~] 0.22 [T~] 0.22 [T~] 1.67 [S2] 1.67
[S3] 0.01 [FIX] 1.71 [SQ] 0.00 [MQ] 0.19 [MF] 0.38 [PE] 1.04 [OF] 1.90 [RN] 1.93 [EO] 2.44 [QQ] 3.62 [CALC] 3.63 [IO] 0.05
   Chr chr22_M33388 89.73% [38]
# [REQ] 3.37 [S1] 0.04
[T~] 0.28 [T~] 0.32 [T~] 0.37 [T~] 1.16 [S2] 1.17
[S3] 0.04 [FIX] 1.26 [SQ] 0.00 [MQ] 0.19 [MF] 0.56 [PE] 1.17 [OF] 1.48 [RN] 1.62 [EO] 2.07 [QQ] 3.85 [CALC] 3.87 [IO] 0.05
   Chr chr22_M33388 92.08% [39]
# [REQ] 3.21 [S1] 0.00
[T~] 0.36 [T~] 0.44 [T~] 0.58 [T~] 0.69 [S2] 0.69
[S3] 0.00 [FIX] 0.70 [SQ] 0.00 [MQ] 0.39 [MF] 0.53 [PE] 1.19 [OF] 1.31 [RN] 1.56 [EO] 1.69 [QQ] 3.39 [CALC] 3.39 [IO] 0.04
   Chr chr22_M33388 94.14% [40]
# [REQ] 2.82 [S1] 0.00
[T~] 0.30 [T~] 0.54 [T~] 0.60 [T~] 0.74 [S2] 0.74
[S3] 0.00 [FIX] 0.75 [SQ] 0.01 [MQ] 0.15 [MF] 0.55 [PE] 1.13 [OF] 1.52 [RN] 1.65 [EO] 2.13 [QQ] 3.65 [CALC] 3.65 [IO] 0.05
   Chr chr22_M33388 96.39% [41]
# [REQ] 3.17 [S1] 0.06
[T~] 0.39 [T~] 0.42 [T~] 0.50 [T~] 1.03 [S2] 1.03
[S3] 0.05 [FIX] 1.16 [SQ] 0.02 [MQ] 0.36 [MF] 0.62 [PE] 0.98 [OF] 1.31 [RN] 1.88 [EO] 1.98 [QQ] 4.16 [CALC] 4.16 [IO] 0.05
   Chr chr22_M33388 98.81% [42]
# [REQ] 3.42 [S1] 0.01
[T~] 0.05 [T~] 0.06 [T~] 0.06 [T~] 1.75 [S2] 1.75
[S3] 0.01 [FIX] 1.78 [SQ] 0.00 [MQ] 0.16 [MF] 0.56 [PE] 1.18 [OF] 1.34 [RN] 1.85 [EO] 2.43 [QQ] 3.81 [CALC] 3.82 [IO] 0.05
   Chr chr22_M33388 100.00% [43]
# [REQ] 1.78 [S1] 0.01
[T~] 0.08 [T~] 0.15 [T~] 0.23 [T~] 0.75 [S2] 0.75
[S3] 0.00 [FIX] 0.77 [SQ] 0.00 [MQ] 0.07 [MF] 0.28 [PE] 0.57 [OF] 0.92 [RN] 0.95 [EO] 1.28 [QQ] 1.91 [CALC] 1.91 [IO] 0.04

Written 42,522,724 lines
[FIX 86473 REP 19362]sequence: 105835
[NUC 20268888 UNK 236387 OP 34108867 LEN 97167150 LOC 6430732 STI 2463]editOp: 158214487
[IDX 13045284 S 185178151]readName: 198223435
mapFlag: 16547932
mapQual: 20476571
quality: 1029975112
pairedEnd: 150710716
optField: 157380812
*/
#include <cstring>
#include <algorithm>
#include <inttypes.h>
#include <stdlib.h>

template<class T> 
class Array {
	T 		*_records;
	size_t  _size;
	size_t  _capacity;
	size_t 	_extend;

public:
	Array (void):
		_records(0), _size(0), _capacity(0), _extend(100)
	{
	}

	Array (size_t c, size_t e = 100):
		_size(0), _capacity(c), _extend(e) 
	{
		_records = new T[_capacity];
	}

	~Array (void) {
		delete[] _records;
	}

public:
	void realloc (size_t sz) {
		_capacity = sz + _extend;

		if (_size > _capacity) _size = _capacity;
		if (!_capacity) { _records = 0; return; }

		T *tmp = new T[_capacity];
		std::copy(_records, _records + _size, tmp);
		delete[] _records;
		_records = tmp;
	}

	void resize (size_t sz) {
		if (sz > _capacity) {
			realloc(sz);
		}
		_size = sz;
	}

	// add element, realloc if needed
	void add (const T &t) {
		if (_size == _capacity)
			realloc(_capacity);
		_records[_size++] = t;
	}

	// add array, realloc if needed
	void add (const T *t, size_t sz) {
		if (_size + sz > _capacity)
			realloc(_capacity + sz);
		std::copy(t, t + sz, _records + _size);
		_size += sz;
	}

	size_t size (void) const {
		return _size;
	}
	
	size_t capacity (void) const {
		return _capacity;
	}

	T *data (void) {
		return _records;
	}

	void set_extend (size_t s) {
		_extend = s;
	}
};

// typedef Array<uint8_t> ByteArray;

#endif