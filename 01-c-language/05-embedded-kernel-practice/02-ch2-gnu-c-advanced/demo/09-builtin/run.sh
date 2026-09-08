#!/bin/bash
# run.sh - all builtin tests
set -e
cd "$(dirname "$0")" || exit 1

GCC="gcc -std=gnu11"
CLANG="clang -std=gnu11"
WARN="-Wall -Wextra"

echo "=========================================="
echo "T1: bit manipulation builtins"
echo "=========================================="
echo "=== gcc compile + run ==="
$GCC $WARN -O2 t1_bitops.c -o t1_gcc && ./t1_gcc
echo "=== clang compile + run ==="
$CLANG $WARN -O2 t1_bitops.c -o t1_clang && ./t1_clang
echo "=== objdump do_popcount (gcc) ==="
objdump -d t1_gcc | sed -n '/<do_popcount>:/,/^\$/p'
echo "=== objdump do_clz (gcc) ==="
objdump -d t1_gcc | sed -n '/<do_clz>:/,/^\$/p'
echo "=== objdump do_ctz (gcc) ==="
objdump -d t1_gcc | sed -n '/<do_ctz>:/,/^\$/p'
echo "=== objdump do_ffs (gcc) ==="
objdump -d t1_gcc | sed -n '/<do_ffs>:/,/^\$/p'
echo "=== objdump do_parity (gcc) ==="
objdump -d t1_gcc | sed -n '/<do_parity>:/,/^\$/p'
echo "=== clang: does it generate same insns? ==="
objdump -d t1_clang | sed -n '/<do_popcount>:/,/^\$/p'

echo ""
echo "=========================================="
echo "T2: byte swap builtins"
echo "=========================================="
$GCC $WARN -O2 t2_bswap.c -o t2_gcc && ./t2_gcc
$CLANG $WARN -O2 t2_bswap.c -o t2_clang && ./t2_clang
echo "=== objdump do_bswap32 (gcc) ==="
objdump -d t2_gcc | sed -n '/<do_bswap32>:/,/^\$/p'
echo "=== objdump do_bswap16 (gcc) ==="
objdump -d t2_gcc | sed -n '/<do_bswap16>:/,/^\$/p'

echo ""
echo "=========================================="
echo "T3: __builtin_constant_p"
echo "=========================================="
$GCC $WARN -O2 t3_constant_p.c -o t3_gcc && ./t3_gcc
$CLANG $WARN -O2 t3_constant_p.c -o t3_clang && ./t3_clang
echo "=== objdump my_memset path (gcc) ==="
objdump -d t3_gcc | sed -n '/<main>:/,/^\$/p' | head -40
echo "=== does -O0 break constant_p? ==="
$GCC $WARN -O0 t3_constant_p.c -o t3_gcc_O0 && ./t3_gcc_O0

echo ""
echo "=========================================="
echo "T4: __builtin_expect - block layout"
echo "=========================================="
$GCC $WARN -O2 t4_expect.c -o t4_gcc && ./t4_gcc
$CLANG $WARN -O2 t4_expect.c -o t4_clang && ./t4_clang
echo "=== check_plain (no expect) ==="
objdump -d t4_gcc | sed -n '/<check_plain>:/,/^\$/p'
echo "=== check_unlikely ==="
objdump -d t4_gcc | sed -n '/<check_unlikely>:/,/^\$/p'
echo "=== check_likely (WRONG direction) ==="
objdump -d t4_gcc | sed -n '/<check_likely>:/,/^\$/p'
echo "=== clang check_unlikely ==="
objdump -d t4_clang | sed -n '/<check_unlikely>:/,/^\$/p'

echo ""
echo "=========================================="
echo "T5: __builtin_prefetch"
echo "=========================================="
$GCC $WARN -O2 t5_prefetch.c -o t5_gcc && ./t5_gcc
$CLANG $WARN -O2 t5_prefetch.c -o t5_clang && ./t5_clang
echo "=== prefetch at -O0 (gcc) ==="
$GCC $WARN -O0 -c t5_prefetch.c -o t5_O0.o
objdump -d t5_O0.o | sed -n '/<prefetch_at_O0>:/,/^\$/p'
echo "=== prefetch at -O2 (gcc) ==="
objdump -d t5_gcc | sed -n '/<demo_prefetch_args>:/,/^\$/p'
echo "=== sum_with_prefetch (does it prefetch?) ==="
objdump -d t5_gcc | sed -n '/<sum_with_prefetch>:/,/^\$/p'

echo ""
echo "=========================================="
echo "T6: control flow builtins"
echo "=========================================="
$GCC $WARN -O2 t6_control.c -o t6_gcc && ./t6_gcc
$CLANG $WARN -O2 t6_control.c -o t6_clang && ./t6_clang
echo "=== do_trap (gcc) ==="
objdump -d t6_gcc | sed -n '/<do_trap>:/,/^\$/p'
echo "=== do_unreachable (gcc) ==="
objdump -d t6_gcc | sed -n '/<do_unreachable>:/,/^\$/p'
echo "=== lie_to_compiler (gcc) ==="
objdump -d t6_gcc | sed -n '/<lie_to_compiler>:/,/^\$/p'
echo "=== clang do_trap ==="
objdump -d t6_clang | sed -n '/<do_trap>:/,/^\$/p'

echo ""
echo "=========================================="
echo "T7: libc builtins - folding"
echo "=========================================="
$GCC $WARN -O2 t7_libc.c -o t7_gcc && ./t7_gcc
$CLANG $WARN -O2 t7_libc.c -o t7_clang && ./t7_clang
echo "=== do_strlen_const (gcc -O2) ==="
objdump -d t7_gcc | sed -n '/<do_strlen_const>:/,/^\$/p'
echo "=== do_memcpy_const (gcc -O2) ==="
objdump -d t7_gcc | sed -n '/<do_memcpy_const>:/,/^\$/p'
echo "=== do_memcpy_large (gcc -O2) ==="
objdump -d t7_gcc | sed -n '/<do_memcpy_large>:/,/^\$/p'
echo "=== -fno-builtin: does strlen fold? ==="
$GCC $WARN -O2 -fno-builtin t7_libc.c -o t7_nobuiltin && ./t7_nobuiltin
objdump -d t7_nobuiltin | sed -n '/<do_strlen_const>:/,/^\$/p'
echo "=== -fno-builtin-strlen: only strlen disabled ==="
$GCC $WARN -O2 -fno-builtin-strlen t7_libc.c -o t7_nobl_strlen && ./t7_nobl_strlen
objdump -d t7_nobl_strlen | sed -n '/<do_strlen_const>:/,/^\$/p'
echo "=== __builtin_strlen survives -fno-builtin? ==="
objdump -d t7_nobuiltin | sed -n '/<my_builtin_strlen>:/,/^\$/p' 2>/dev/null || echo "(function not found)"

echo ""
echo "=========================================="
echo "T8: overflow builtins"
echo "=========================================="
$GCC $WARN -O2 t8_overflow.c -o t8_gcc && ./t8_gcc
$CLANG $WARN -O2 t8_overflow.c -o t8_clang && ./t8_clang
echo "=== safe_add (gcc) ==="
objdump -d t8_gcc | sed -n '/<safe_add>:/,/^\$/p'
echo "=== safe_mul (gcc) ==="
objdump -d t8_gcc | sed -n '/<safe_mul>:/,/^\$/p'

echo ""
echo "=========================================="
echo "T9: type/alignment builtins"
echo "=========================================="
$GCC $WARN -O2 t9_types.c -o t9_gcc && ./t9_gcc
$CLANG $WARN -O2 t9_types.c -o t9_clang 2>&1 | head -5; ./t9_clang 2>/dev/null || echo "(clang compiled with warnings)"
echo "=== sum_aligned vs sum_plain (gcc) ==="
objdump -d t9_gcc | sed -n '/<sum_aligned>:/,/^\$/p'
echo "--- plain ---"
objdump -d t9_gcc | sed -n '/<sum_plain>:/,/^\$/p'

echo ""
echo "=========================================="
echo "T10: -fno-builtin effect"
echo "=========================================="
echo "=== normal ==="
$GCC $WARN -O2 t10_fno_builtin.c -o t10_gcc && ./t10_gcc
echo "=== -fno-builtin ==="
$GCC $WARN -O2 -fno-builtin t10_fno_builtin.c -o t10_nob && ./t10_nob
echo "=== -fno-builtin-strlen ==="
$GCC $WARN -O2 -fno-builtin-strlen t10_fno_builtin.c -o t10_nobl && ./t10_nobl
echo "=== objdump: strlen_const (normal) ==="
objdump -d t10_gcc | sed -n '/<my_strlen_const>:/,/^\$/p'
echo "=== objdump: strlen_const (-fno-builtin) ==="
objdump -d t10_nob | sed -n '/<my_strlen_const>:/,/^\$/p'

echo ""
echo "=========================================="
echo "T11: GCC vs Clang matrix"
echo "=========================================="
echo "=== gcc ==="
$GCC $WARN -O2 t11_matrix.c -o t11_gcc 2>&1 | head -10
./t11_gcc 2>/dev/null || echo "(compile failed or runtime error)"
echo "=== clang ==="
$CLANG $WARN -O2 t11_matrix.c -o t11_clang 2>&1 | head -10
./t11_clang 2>/dev/null || echo "(compile failed or runtime error)"

echo ""
echo "=========================================="
echo "T12: C23 builtins"
echo "=========================================="
echo "=== gcc -std=c2x ==="
gcc -std=c2x $WARN -O2 t12_c23.c -o t12_gcc 2>&1 | head -10
./t12_gcc 2>/dev/null || echo "(run failed)"
echo "=== clang -std=c2x ==="
clang -std=c2x $WARN -O2 t12_c23.c -o t12_clang 2>&1 | head -10
./t12_clang 2>/dev/null || echo "(run failed)"
echo "=== freestanding: does -ffreestanding kill builtins? ==="
$GCC $WARN -O2 -ffreestanding -c t7_libc.c -o t7_free.o 2>&1 | head -10
echo "(exit: $?)"

echo ""
echo "=========================================="
echo "DONE"
echo "=========================================="
