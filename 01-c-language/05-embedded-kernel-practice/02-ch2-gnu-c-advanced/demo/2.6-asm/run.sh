#!/bin/bash
# 3.6 inline assembly — full test matrix (gcc 13.3 / clang 18.1.3, x86-64)
cd "$(dirname "$0")" || exit 1
GCC=gcc
CLANG=clang
CFLAGS="-std=gnu11 -Wall -Wextra -O2 -fno-pie -no-pie"
# Disasm flags: prevent inlining so each function survives as a symbol
DFLAGS="-std=gnu11 -O0 -fno-inline -fno-ipa-cp -fno-pie -no-pie -c"
SEP() { printf '\n========== %s ==========\n' "$1"; }

run_one() {
  local f="$1" ; local base="${f%.c}"
  SEP "$base (gcc)"
  $GCC $CFLAGS "$f" -o "${base}_gcc" 2>&1 | head -20
  ./"${base}_gcc" 2>&1 | head -15
  SEP "$base (clang)"
  $CLANG $CFLAGS "$f" -o "${base}_clang" 2>&1 | head -20
  ./"${base}_clang" 2>&1 | head -15
}

# ---- T1: basic vs extended ----
run_one t1_basic_vs_ext.c

# ---- T2: named operands ----
run_one t2_named.c
SEP "T2 DEMO_BAD (mix positional+named, expect error)"
echo "--- gcc ---"; $GCC $CFLAGS -DDEMO_BAD -c t2_named.c -o /dev/null 2>&1 | head -8
echo "--- clang ---"; $CLANG $CFLAGS -DDEMO_BAD -c t2_named.c -o /dev/null 2>&1 | head -8

# ---- T3: output constraints ----
run_one t3_output_constraints.c
SEP "T3 disasm: rw_reg (+r, no extra load/store)"
$GCC $DFLAGS t3_output_constraints.c -o t3d.o 2>/dev/null
objdump -d t3d.o | sed -n '/<rw_reg>:/,/^$/p' | head -8
echo "--- bug_alias (=r but reads %0 input too) ---"
objdump -d t3d.o | sed -n '/<bug_alias>:/,/^$/p' | head -10

# ---- T4: input constraints ----
run_one t4_input_constraints.c

# ---- T5: earlyclobber + commutative ----
run_one t5_earlyclobber.c
SEP "T5 disasm: fix_amp (=&r output not aliased with inputs)"
$GCC $DFLAGS t5_earlyclobber.c -o t5d.o 2>/dev/null
echo "--- fix_amp (=&r) ---"
objdump -d t5d.o | sed -n '/<fix_amp>:/,/^$/p' | head -10
echo "--- bug_no_amp (=r, may alias) ---"
objdump -d t5d.o | sed -n '/<bug_no_amp>:/,/^$/p' | head -10

# ---- T6: clobber ----
run_one t6_clobber.c
SEP "T6 disasm: forget_clobber (missing rax)"
$GCC $DFLAGS t6_clobber.c -o t6d.o 2>/dev/null
objdump -d t6d.o | sed -n '/<forget_clobber>:/,/^$/p' | head -12

# ---- T7: volatile semantics ----
run_one t7_volatile.c
SEP "T7 disasm: dead_asm (no vol) vs kept_asm (vol)"
$GCC $DFLAGS t7_volatile.c -o t7d.o 2>/dev/null
echo "--- dead_asm (no volatile, output unused) ---"
objdump -d t7d.o | sed -n '/<dead_asm>:/,/^$/p' | head -8
echo "--- kept_asm (volatile, output unused) ---"
objdump -d t7d.o | sed -n '/<kept_asm>:/,/^$/p' | head -8

# ---- T8: -O0 vs -O2 ----
run_one t8_opt_levels.c
SEP "T8 disasm: demo_bug at -O0 vs -O2 (missing rax clobber)"
echo "--- demo_bug @ -O0 ---"
$GCC -std=gnu11 -Wall -O0 -fno-inline -fno-pie -no-pie -c t8_opt_levels.c -o t8_O0.o 2>/dev/null
objdump -d t8_O0.o | sed -n '/<demo_bug>:/,/^$/p' | head -15
echo "--- demo_bug @ -O2 (gcc) ---"
$GCC -std=gnu11 -Wall -O2 -fno-inline -fno-ipa-cp -fno-pie -no-pie -c t8_opt_levels.c -o t8_O2g.o 2>/dev/null
objdump -d t8_O2g.o | sed -n '/<demo_bug>:/,/^$/p' | head -12
echo "--- demo_bug @ -O2 (clang) ---"
$CLANG -std=gnu11 -Wall -O2 -fno-inline -fno-ipa-cp -fno-pie -no-pie -c t8_opt_levels.c -o t8_O2c.o 2>/dev/null
objdump -d t8_O2c.o | sed -n '/<demo_bug>:/,/^$/p' | head -12

# ---- T9: asm goto ----
run_one t9_asm_goto.c
SEP "T9 disasm: try_feature (asm goto, jump to label)"
$GCC $DFLAGS t9_asm_goto.c -o t9d.o 2>/dev/null
objdump -d t9d.o | sed -n '/<try_feature>:/,/^$/p' | head -15

# ---- T10: bad constraints ----
run_one t10_bad_constraints.c
SEP "T10 DEMO_BAD (bad_impossible: i with runtime var)"
echo "--- gcc ---"; $GCC $CFLAGS -DDEMO_BAD -c t10_bad_constraints.c -o /dev/null 2>&1 | head -8
echo "--- clang ---"; $CLANG $CFLAGS -DDEMO_BAD -c t10_bad_constraints.c -o /dev/null 2>&1 | head -8
SEP "T10 DEMO_BAD (bad_size: movl with 64-bit operand)"
echo "--- gcc ---"; $GCC $CFLAGS -DDEMO_BAD -c t10_bad_constraints.c -o /dev/null 2>&1 | grep -i "error\|suffix\|register" | head -5
echo "--- clang ---"; $CLANG $CFLAGS -DDEMO_BAD -c t10_bad_constraints.c -o /dev/null 2>&1 | grep -i "error\|invalid" | head -5

# ---- T11: ABI ----
run_one t11_abi.c
SEP "T11 disasm: six_args (rdi,rsi,rdx,rcx,r8,r9)"
$GCC $DFLAGS t11_abi.c -o t11d.o 2>/dev/null
objdump -d t11d.o | sed -n '/<six_args>:/,/^$/p' | head -15
echo "--- force_rdi (D constraint -> edi) ---"
objdump -d t11d.o | sed -n '/<force_rdi>:/,/^$/p' | head -8

# ---- T12: asm calls C (.S + .c) ----
SEP "T12: .S calls C function"
$GCC $CFLAGS t12_c_side.c t12_asm_calls_c.S -o t12_gcc 2>&1 | head -10
./t12_gcc 2>&1 | head -5
$CLANG $CFLAGS t12_c_side.c t12_asm_calls_c.S -o t12_clang 2>&1 | head -10
./t12_clang 2>&1 | head -5
echo "--- disasm asm_call_c ---"
objdump -d t12_gcc | sed -n '/<asm_call_c>:/,/^$/p' | head -12

# ---- T13: kernel patterns ----
run_one t13_kernel_patterns.c
SEP "T13 disasm: barrier / rdtsc / xchg / cmpxchg"
$GCC $DFLAGS t13_kernel_patterns.c -o t13d.o 2>/dev/null
echo "--- mb (mfence) ---"
objdump -d t13d.o | sed -n '/<mb>:/,/^$/p' | head -5
echo "--- rdtsc ---"
objdump -d t13d.o | sed -n '/<rdtsc>:/,/^$/p' | head -6
echo "--- xchg ---"
objdump -d t13d.o | sed -n '/<xchg>:/,/^$/p' | head -8
echo "--- cmpxchg ---"
objdump -d t13d.o | sed -n '/<cmpxchg>:/,/^$/p' | head -10

SEP "ALL DONE"
