BITS 64
default rel

%include "nasm-utils-inc.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

; aligns and declares the global label for the bench with the given name
; also potentally checks the ABI compliance (if enabled)
%macro define_func 1
abi_checked_function %1
%endmacro

; define a test func that unrolls the loop by 100
; with the given body instruction
; %1 - function name
; %2 - init instruction (e.g., xor out the variable you'll add to)
; %3 - loop body instruction
%macro test_func 3
define_func %1
%2
.top:
times 100 %3
sub rdi, 100
jnz .top
ret
%endmacro

test_func scalar_iadd, {xor eax, eax}, {add rax, rax}
test_func avx128_iadd,  {vpcmpeqd xmm0, xmm0, xmm0}, {vpaddq  xmm0, xmm0, xmm0}
test_func avx128_imul,  {vpcmpeqd xmm0, xmm0, xmm0}, {vpmuldq xmm0, xmm0, xmm0}
test_func avx128_fma ,  {vpxor    xmm0, xmm0, xmm0}, {vfmadd132pd xmm0, xmm0, xmm0}
test_func avx256_iadd,  {vpcmpeqd ymm0, ymm0, ymm0}, {vpaddq  ymm0, ymm0, ymm0}
test_func avx256_imul,  {vpcmpeqd ymm0, ymm0, ymm0}, {vpmuldq ymm0, ymm0, ymm0}
test_func avx256_fma ,  {vpxor    xmm0, xmm0, xmm0}, {vfmadd132pd ymm0, ymm0, ymm0}
test_func avx512_iadd,  {vpcmpeqd ymm0, ymm0, ymm0}, {vpaddq  zmm0, zmm0, zmm0}
test_func avx512_imul,  {vpcmpeqd ymm0, ymm0, ymm0}, {vpmuldq zmm0, zmm0, zmm0}
test_func avx512_fma ,  {vpxor    xmm0, xmm0, xmm0}, {vfmadd132pd zmm0, zmm0, zmm0}

; this is like test_func, but it uses 10 parallel chains of instructions,
; unrolled 10 times, so (probably) max throughput
; %1 - function name
; %2 - init instruction (e.g., xor out the variable you'll add to)
; %3 - register base like xmm, ymm, zmm
; %3 - loop body instruction only (no operands)
%macro test_func_tput 5
define_func %1

; init
%assign r 0
%rep 10
%2 %3 %+ r, %5
%assign r (r+1)
%endrep

.top:
%rep 10
%assign r 0
%rep 10
%4 %3 %+ r, %3 %+ r, %3 %+ r
;vfmadd132pd ymm %+ r, ymm0, ymm0
%assign r (r+1)
%endrep
%endrep
sub rdi, 100
jnz .top
ret
%endmacro

test_func_tput avx256_fma_t ,  vbroadcastsd, ymm, vfmadd132pd, [zero_dp]
test_func_tput avx512_fma_t ,  vbroadcastsd, zmm, vfmadd132pd, [zero_dp]

; this is like test_func except that the 100x unrolled loop instruction is
; always a serial scalar add, while the passed instruction to test is only
; executed once per loop (so at a ratio of 1:100 for the scalar adds). This
; test the effect of an "occasional" AVX instruction.
; %1 - function name
; %2 - init instruction (e.g., xor out the variable you'll add to)
; %3 - loop body instruction
%macro test_func_sparse 3
define_func %1
%2
xor eax, eax
.top:
%3
times 100 add eax, eax
sub rdi, 100
jnz .top
ret
%endmacro

test_func_sparse avx128_mov_sparse,       {vbroadcastsd ymm0, [one_dp]}, {vmovdqa xmm0, xmm0}
test_func_sparse avx256_mov_sparse,       {vbroadcastsd ymm0, [one_dp]}, {vmovdqa ymm0, ymm0}
test_func_sparse avx512_mov_sparse,       {vbroadcastsd zmm0, [one_dp]}, {vmovdqa32 zmm0, zmm0}
test_func_sparse avx512_mov_merge_sparse, {vbroadcastsd zmm0, [one_dp]}, {vmovdqa32 zmm0{k1}, zmm0}

test_func_sparse avx128_fma_sparse, {vbroadcastsd ymm0, [zero_dp]}, {vfmadd132pd xmm0, xmm0, xmm0 }
test_func_sparse avx256_fma_sparse, {vbroadcastsd ymm0, [zero_dp]}, {vfmadd132pd ymm0, ymm0, ymm0 }
test_func_sparse avx512_fma_sparse, {vbroadcastsd zmm0, [zero_dp]}, {vfmadd132pd zmm0, zmm0, zmm0 }


GLOBAL zeroupper:function
zeroupper:
vzeroupper
ret

zero_dp: dq 0.0
one_dp:  dq 1.0



