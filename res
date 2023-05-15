CPUID highest leaf    : [1fh]
Running as root       : [YES]
MSR reads supported   : [NO ]
CPU pinning enabled   : [YES]
CPU supports zeroupper: [YES]
CPU supports AVX2     : [YES]
CPU supports AVX-512F : [YES]
CPU supports AVX-512VL: [YES]
CPU supports AVX-512BW: [YES]
CPU supports AVX-512CD: [YES]
cpuid = eax = 0, ebx = 0, ecx = 0, edx = 0
cpu: family = 6, model = 143, stepping = 8
tsc_freq = 2600.1 MHz (from calibration loop)
CPU brand string: Intel(R) Xeon(R) Platinum 8457C
32 available CPUs: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31]
16 physical cores: [0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30]

 TILE DATA USE SET - OK 

Will test up to 16 CPUs
Cores | ID           | Description                 | OVRLP3 | Mops
1     | scalar_iadd  | Scalar integer adds         |  1.000 | 3094
1     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 3093
1     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3094
1     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 6188
1     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 6180
1     | amx_int8     | AMX 8-bit integer           |  1.000 |  193

Cores | ID           | Description                 | OVRLP3 |       Mops
2     | scalar_iadd  | Scalar integer adds         |  1.000 | 2855, 2851
2     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2760, 2757
2     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3011, 3016
2     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2968, 2975
2     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3076, 3058
2     | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96

Cores | ID           | Description                 | OVRLP3 |             Mops
3     | scalar_iadd  | Scalar integer adds         |  1.000 | 2855, 2852, 3094
3     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2760, 2760, 3094
3     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3017, 3016, 3093
3     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2947, 2924, 6186
3     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 2977, 3076, 6185
3     | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,  193

Cores | ID           | Description                 | OVRLP3 |                   Mops
4     | scalar_iadd  | Scalar integer adds         |  1.000 | 2850, 2853, 2854, 2854
4     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2756, 2759, 2759, 2758
4     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3014, 3016, 3010, 3011
4     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2958, 2955, 3010, 3017
4     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3072, 3080, 3090, 3083
4     | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96

Cores | ID           | Description                 | OVRLP3 |                         Mops
5     | scalar_iadd  | Scalar integer adds         |  1.000 | 2856, 2854, 2855, 2853, 3093
5     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2759, 2756, 2757, 2755, 3093
5     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3014, 3008, 3016, 3014, 3093
5     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2968, 2938, 3003, 2964, 6187
5     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3020, 2990, 3094, 3094, 6177
5     | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,  193

Cores | ID           | Description                 | OVRLP3 |                               Mops
6     | scalar_iadd  | Scalar integer adds         |  1.000 | 2852, 2854, 2851, 2856, 2856, 2851
6     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2758, 2758, 2761, 2760, 2758, 2758
6     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3018, 3017, 3012, 3012, 3017, 3016
6     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2945, 2978, 2980, 3044, 2998, 2992
6     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3016, 2978, 3079, 3079, 3094, 3081
6     | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96

Cores | ID           | Description                 | OVRLP3 |                                     Mops
7     | scalar_iadd  | Scalar integer adds         |  1.000 | 2851, 2854, 2850, 2855, 2853, 2852, 3094
7     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2757, 2760, 2755, 2756, 2759, 2757, 3094
7     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3013, 3017, 3013, 3013, 3009, 3012, 3094
7     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 3003, 2993, 2989, 3004, 2972, 3000, 6181
7     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 2966, 3025, 2993, 3080, 3077, 3095, 6186
7     | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,  193

Cores | ID           | Description                 | OVRLP3 |                                           Mops
8     | scalar_iadd  | Scalar integer adds         |  1.000 | 2854, 2854, 2850, 2855, 2853, 2853, 2854, 2853
8     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2756, 2756, 2761, 2758, 2757, 2759, 2760, 2758
8     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3015, 3016, 3017, 3016, 3012, 3013, 3012, 3014
8     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2992, 2991, 2990, 2994, 2989, 2993, 3001, 3002
8     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3059, 3053, 3072, 3084, 3080, 3082, 3067, 3076
8     | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96

Cores | ID           | Description                 | OVRLP3 |                                                 Mops
9     | scalar_iadd  | Scalar integer adds         |  1.000 | 2853, 2852, 2851, 2854, 2854, 2853, 2852, 2856, 3093
9     | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2757, 2758, 2756, 2754, 2760, 2755, 2760, 2759, 3093
9     | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3015, 3017, 3017, 3013, 3015, 3015, 3016, 3016, 3093
9     | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2928, 2975, 2992, 2991, 3020, 2971, 3025, 3000, 6190
9     | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 2944, 3048, 3072, 3075, 3081, 3089, 3109, 3094, 6185
9     | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96,  191

Cores | ID           | Description                 | OVRLP3 |                                                       Mops
10    | scalar_iadd  | Scalar integer adds         |  1.000 | 2854, 2855, 2852, 2853, 2854, 2854, 2854, 2849, 2852, 2855
10    | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2761, 2758, 2760, 2759, 2757, 2759, 2758, 2758, 2755, 2756
10    | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3015, 3009, 3017, 3012, 3016, 3017, 3016, 3014, 3010, 3013
10    | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2997, 2895, 2999, 2986, 2998, 2970, 2991, 2980, 2988, 2991
10    | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3064, 3091, 3070, 3051, 3089, 3083, 3079, 3080, 3075, 3067
10    | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96,   96,   96

Cores | ID           | Description                 | OVRLP3 |                                                             Mops
11    | scalar_iadd  | Scalar integer adds         |  1.000 | 2854, 2854, 2856, 2854, 2854, 2854, 2854, 2853, 2849, 2856, 3093
11    | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2760, 2757, 2757, 2756, 2756, 2758, 2754, 2761, 2756, 2758, 3093
11    | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3016, 3010, 3019, 3017, 3009, 3011, 3003, 3006, 3015, 3008, 3093
11    | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2957, 2986, 3030, 2999, 3015, 2980, 2999, 2976, 3005, 2979, 6187
11    | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3018, 3078, 3081, 3077, 3101, 3093, 3096, 3095, 3088, 3082, 6185
11    | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,  193

Cores | ID           | Description                 | OVRLP3 |                                                                   Mops
12    | scalar_iadd  | Scalar integer adds         |  1.000 | 2852, 2854, 2853, 2852, 2854, 2854, 2855, 2853, 2851, 2854, 2852, 2852
12    | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2756, 2758, 2759, 2758, 2759, 2757, 2757, 2761, 2758, 2757, 2761, 2754
12    | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3018, 3016, 3015, 3015, 3008, 3007, 3010, 3011, 3010, 3009, 3017, 3018
12    | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 3003, 2976, 3030, 2970, 2984, 2998, 2989, 3014, 2974, 2979, 2995, 2979
12    | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 2909, 3087, 3103, 3070, 3105, 3098, 3080, 3092, 3081, 3093, 3074, 3093
12    | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96

Cores | ID           | Description                 | OVRLP3 |                                                                         Mops
13    | scalar_iadd  | Scalar integer adds         |  1.000 | 2851, 2852, 2852, 2854, 2850, 2853, 2855, 2854, 2852, 2854, 2853, 2852, 3093
13    | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2758, 2759, 2758, 2758, 2756, 2759, 2759, 2757, 2757, 2757, 2757, 2756, 3093
13    | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3014, 3016, 3017, 3019, 3019, 3012, 3014, 3013, 3018, 3015, 3013, 3015, 3093
13    | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2973, 2996, 2999, 2961, 3025, 2970, 2973, 3017, 2989, 2989, 2988, 2993, 6186
13    | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3070, 3074, 3095, 3111, 3083, 3083, 3084, 3067, 3079, 3091, 3087, 3090, 6186
13    | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,  193

Cores | ID           | Description                 | OVRLP3 |                                                                               Mops
14    | scalar_iadd  | Scalar integer adds         |  1.000 | 2854, 2853, 2853, 2853, 2851, 2855, 2853, 2854, 2855, 2850, 2855, 2852, 2854, 2852
14    | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2760, 2756, 2760, 2756, 2760, 2758, 2757, 2757, 2756, 2758, 2756, 2760, 2758, 2757
14    | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3011, 3015, 3018, 3018, 3014, 3013, 3012, 3013, 3009, 3009, 3010, 3015, 3011, 3007
14    | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2989, 3013, 2986, 2998, 3010, 3006, 3023, 3007, 3013, 2996, 3023, 3026, 3001, 2993
14    | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3065, 3088, 3081, 3027, 3079, 3078, 3081, 3100, 3080, 3083, 3090, 3089, 3082, 3074
14    | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96

Cores | ID           | Description                 | OVRLP3 |                                                                                     Mops
15    | scalar_iadd  | Scalar integer adds         |  1.000 | 2852, 2853, 2853, 2853, 2852, 2854, 2852, 2856, 2852, 2853, 2855, 2854, 2853, 2853, 3093
15    | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2756, 2758, 2759, 2763, 2757, 2760, 2756, 2760, 2758, 2758, 2759, 2757, 2758, 2759, 3094
15    | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3010, 3009, 3011, 3011, 3013, 3010, 3017, 3017, 3016, 3011, 3014, 3013, 3008, 3009, 3094
15    | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2989, 3010, 2979, 3009, 3006, 2972, 2998, 3003, 3002, 3021, 2983, 3021, 3003, 2943, 6184
15    | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 2981, 3018, 3096, 3089, 3086, 3083, 3082, 3066, 3076, 3085, 3082, 3093, 3081, 3084, 6181
15    | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,  193

Cores | ID           | Description                 | OVRLP3 |                                                                                           Mops
16    | scalar_iadd  | Scalar integer adds         |  1.000 | 2853, 2851, 2853, 2855, 2853, 2852, 2852, 2854, 2855, 2852, 2853, 2853, 2855, 2852, 2855, 2854
16    | avx256_iadd  | 256-bit integer serial adds |  1.000 | 2758, 2760, 2760, 2761, 2758, 2759, 2761, 2756, 2759, 2757, 2758, 2758, 2756, 2760, 2758, 2761
16    | avx512_iadd  | 512-bit integer serial adds |  1.000 | 3014, 3017, 3010, 3009, 3009, 3007, 3019, 3022, 3013, 3014, 3008, 3008, 3018, 3017, 3016, 3013
16    | avx256_fma_t | 256-bit parallel DP FMAs    |  1.000 | 2945, 2957, 2989, 2998, 3012, 2980, 2951, 2901, 2985, 3017, 2979, 2979, 2982, 3002, 2991, 2985
16    | avx512_fma_t | 512-bit parallel DP FMAs    |  1.000 | 3014, 3079, 3065, 3071, 3083, 3079, 3086, 3079, 3093, 3089, 3084, 3077, 3095, 3090, 3094, 3089
16    | amx_int8     | AMX 8-bit integer           |  1.000 |   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96,   96

