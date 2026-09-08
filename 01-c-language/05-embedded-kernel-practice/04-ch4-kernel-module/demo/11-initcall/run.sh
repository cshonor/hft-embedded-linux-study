#!/bin/bash
# run.sh —— initcall 机制用户态模拟实测
# 环境要求: gcc + binutils (ld, objdump, readelf, nm)
set -e
cd "$(dirname "$0")" || exit 1

CC=gcc
CFLAGS="-std=gnu11 -Wall -Wextra -O2"
LDFLAGS="-no-pie -Wl,-z,norelro"   # 禁 PIE 让自定义链接脚本的固定地址生效

echo "================================================================"
echo "T1: built-in module_init (section attribute)"
echo "================================================================"
$CC $CFLAGS -c t1_built_in.c -o t1.o
echo "--- readelf: .initcall6.init section ---"
readelf -S t1.o | grep -A1 initcall
echo "--- nm: initcall symbols ---"
nm t1.o | grep sim_initcall
echo "--- 链接运行 ---"
$CC $CFLAGS $LDFLAGS t1.o -o t1
./t1
echo

echo "================================================================"
echo "T2: do_initcalls (linker script + section collection)"
echo "================================================================"
$CC $CFLAGS -c t2_do_initcalls.c -o t2.o
echo "--- 用自定义链接脚本链接 ---"
$CC $CFLAGS $LDFLAGS -T initcall.lds t2.o -o t2
echo "--- readelf: .initcall section ---"
readelf -S t2 | grep -A1 initcall
echo "--- 运行 ---"
./t2
echo

echo "================================================================"
echo "T3: initcall levels (0-7)"
echo "================================================================"
$CC $CFLAGS -c t3_levels.c -o t3.o
$CC $CFLAGS $LDFLAGS -T initcall_levels.lds t3.o -o t3
echo "--- readelf: 8 level sections ---"
readelf -S t3 | grep initcall
echo "--- 运行 ---"
./t3
echo

echo "================================================================"
echo "T4: KEEP() vs gc-sections"
echo "================================================================"
echo "--- 不带 --gc-sections ---"
$CC $CFLAGS $LDFLAGS -T keep_test.lds t4_keep.c -o t4_nogc
./t4_nogc
echo "--- 带 --gc-sections 但有 KEEP (函数代码被删 → segfault 是教学点) ---"
$CC $CFLAGS $LDFLAGS -Wl,--gc-sections -T keep_test.lds t4_keep.c -o t4_gc_keep || true
./t4_gc_keep || echo "  -> segfault! KEEP 保留了指针，但函数代码被 gc 删了"
echo "--- nm: 确认函数符号是否存在 ---"
nm t4_gc_keep 2>/dev/null | grep -E "k1|k2|k3|keep" || echo "  函数符号被 gc 删除"
echo "--- size 对比 ---"
ls -l t4_nogc t4_gc_keep | awk '{print $5, $9}'
echo

echo "================================================================"
echo "T5: module_init alias (dlsym simulation)"
echo "================================================================"
$CC $CFLAGS -rdynamic -ldl t5_module_alias.c -o t5
echo "--- nm: init_module vs my_driver_init ---"
nm t5 | grep -E "init_module|my_driver_init"
echo "--- 运行 ---"
./t5
echo

echo "================================================================"
echo "T6: __init section lifecycle"
echo "================================================================"
$CC $CFLAGS -c t6_init_free.c -o t6.o
$CC $CFLAGS $LDFLAGS -T initcall.lds t6.o -o t6
echo "--- readelf: .init.text section ---"
readelf -S t6 | grep -E "init.text|initcall"
echo "--- 运行 ---"
./t6
echo

echo "================================================================"
echo "T7: initcall failure handling"
echo "================================================================"
$CC $CFLAGS -c t7_failure.c -o t7.o
$CC $CFLAGS $LDFLAGS -T initcall.lds t7.o -o t7
./t7
echo

echo "================================================================"
echo "T8: PREL32 vs absolute address"
echo "================================================================"
$CC $CFLAGS t8_prel32.c -o t8
./t8
echo

echo "================================================================"
echo "T9: module_init dual version (MODULE vs builtin)"
echo "================================================================"
$CC $CFLAGS -c t9_dual_version.c -o t9.o
$CC $CFLAGS $LDFLAGS -T initcall.lds t9.o -o t9
echo "--- nm: init_module (alias) ---"
nm t9 | grep -E "init_module|my_mod_init|__builtin_initcall"
echo "--- 运行 ---"
./t9
echo

echo "================================================================"
echo "T10: initcall link order (multi-source)"
echo "================================================================"
$CC $CFLAGS -c t10_a.c -o t10_a.o
$CC $CFLAGS -c t10_b.c -o t10_b.o
$CC $CFLAGS -c t10_c.c -o t10_c.o
$CC $CFLAGS -c t10_order.c -o t10_order.o
echo "--- 链接顺序: a b c order ---"
$CC $CFLAGS $LDFLAGS -T initcall.lds t10_a.o t10_b.o t10_c.o t10_order.o -o t10_abc
./t10_abc
echo "--- 链接顺序: c b a order ---"
$CC $CFLAGS $LDFLAGS -T initcall.lds t10_c.o t10_b.o t10_a.o t10_order.o -o t10_cba
./t10_cba
echo

echo "================================================================"
echo "ALL TESTS DONE"
echo "================================================================"
