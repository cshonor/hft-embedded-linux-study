#!/bin/bash
# run.sh —— 内核链表机制全量实测 (用 wsl --exec 调用, 避免 profile 加载卡住)
cd "$(dirname "$0")" || exit 1

CC=gcc
CFLAGS="-std=gnu11 -Wall -Wextra -O2"

echo "============================================================"
echo "  内核链表机制全量实测 (gcc $(gcc -dumpversion), $(uname -m))"
echo "============================================================"

run() {
    local name=$1; local src=$2; local extra=$3
    echo ""
    echo ">>> $name"
    echo "------------------------------------------------------------"
    $CC $CFLAGS $extra "$src" -o "${src%.c}_run" 2>&1 | head -10
    timeout 5 ./"${src%.c}_run" 2>&1
    echo "(exit=$?)"
}

run "T1: 侵入式 vs 外挂式"        t1_invasive_vs_external.c
run "T2: 一个结构体挂多张链表"     t2_multi_list.c
run "T3: list_del POISON vs list_del_init" t3_list_ops.c
run "T4: 遍历宏家族"               t4_iter_macros.c
run "T5: hlist (哈希链表)"         t5_hlist.c
run "T6: 零开销验证"               t6_zerooverhead.c
run "T7: 内核真实用例模拟"         t7_kernel_patterns.c
run "T8: 宏展开实测"               t8_macro_expand.c
run "T9: RCU 链表"                 t9_rcu_list.c
run "T10: 边界与坑点"              t10_pitfalls.c "-Wno-array-bounds"
run "T11: list vs slist vs array"  t11_compare.c

# T6: 反汇编
echo ""
echo ">>> T6: 零开销反汇编"
echo "------------------------------------------------------------"
$CC $CFLAGS -fno-inline -fno-ipa-cp -c t6_zerooverhead.c -o t6.o 2>&1 | head -3
echo "--- get_host (预期: lea -0x48(%rdi),%rax + ret) ---"
objdump -d t6.o | sed -n '/<get_host>:/,/^$/p' | head -6
echo "--- get_next ---"
objdump -d t6.o | sed -n '/<get_next>:/,/^$/p' | head -6
echo "--- sum_irqs (遍历展开) ---"
objdump -d t6.o | sed -n '/<sum_irqs>:/,/^$/p' | head -15

echo ""
echo "============================================================"
echo "  全量实测完成"
echo "============================================================"
