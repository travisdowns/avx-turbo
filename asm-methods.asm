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


GLOBAL zeroupper:function
zeroupper:
vzeroupper
ret

zero_dp: dq 0.0
one_dp:  dq 1.0



