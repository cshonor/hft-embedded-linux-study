#!/bin/bash
# format attribute: full measurement sweep
cd "$(dirname "$0")" || exit 1

GCC="gcc -std=gnu11"
CLANG="clang -std=gnu11"
BASE="-Wall -Wformat=2"

sweep() {           # $1 = file base name
    echo "==================== $1 ===================="
    echo "---------- gcc 13.3 ----------"
    $GCC $BASE -c "$1.c" -o /dev/null 2>&1
    echo "---------- clang 18 ----------"
    $CLANG $BASE -c "$1.c" -o /dev/null 2>&1
    echo
}

sweep t1_basic
sweep t2_valist
sweep t3_archetype

echo "==================== t4_woptions (-O2 + full format family) ===================="
EXTRA="-Wformat-signedness -Wformat-truncation -Wformat-overflow -O2"
echo "---------- gcc -O2 ----------"
$GCC $BASE $EXTRA -c t4_woptions.c -o /dev/null 2>&1
echo "---------- clang -O2 ----------"
$CLANG $BASE $EXTRA -c t4_woptions.c -o /dev/null 2>&1
echo
echo "---------- gcc -O2 -D_FORTIFY_SOURCE=2 (%n) ----------"
$GCC $BASE -D_FORTIFY_SOURCE=2 -O2 -c t4_woptions.c -o /dev/null 2>&1
echo

sweep t5_scope

echo "==================== t6_zerooverhead: does it vanish at -O2? ===================="
$GCC -O2 -c t6_zerooverhead.c -o t6.o 2>&1
echo "---------- objdump -d t6.o (main) ----------"
objdump -d t6.o | sed -n '/<main>:/,/^$/p'
echo
echo "---------- symbols referenced by t6.o ----------"
nm -u t6.o
