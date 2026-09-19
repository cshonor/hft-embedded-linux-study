#!/bin/bash
cd "$(dirname "$0")" || exit 1
BASE="-Wall -Wformat=2"

echo "==================== T7a: which archetypes exist? ===================="
echo "---------- gcc ----------"
gcc -std=gnu11 $BASE -c t7_archetype2.c -o /dev/null 2>&1
echo "---------- clang ----------"
clang -std=gnu11 $BASE -c t7_archetype2.c -o /dev/null 2>&1
echo

echo "==================== T7b: does -std= change the archetype? ===================="
for std in c89 c99 c11 c17 gnu89 gnu11 gnu17; do
    echo "---------- gcc -std=$std ----------"
    gcc -std=$std -Wall -Wformat=2 -c t7_dialect.c -o /dev/null 2>&1 | grep -E "warning|error" | sed 's/^/    /'
done
echo

echo "==================== T7c: security / %n / fortify ===================="
echo "---------- gcc ----------"
gcc -std=gnu11 $BASE -c t7_security.c -o /dev/null 2>&1
echo "---------- gcc -D_FORTIFY_SOURCE=2 -O2 ----------"
gcc -std=gnu11 $BASE -D_FORTIFY_SOURCE=2 -O2 -c t7_security.c -o /dev/null 2>&1
echo "---------- clang ----------"
clang -std=gnu11 $BASE -c t7_security.c -o /dev/null 2>&1
echo

echo "==================== T7d: kernel extended conversions ===================="
echo "---------- gcc ----------"
gcc -std=gnu11 $BASE -c t7_kernel.c -o /dev/null 2>&1
echo "---------- clang ----------"
clang -std=gnu11 $BASE -c t7_kernel.c -o /dev/null 2>&1
