#!/usr/bin/env bash
# run.sh —— 一键编译运行 demo/13-module 全部 5 个实验
set -e
cd "$(dirname "$0")"
CC=${CC:-gcc}

echo "############ 编译 ############"
$CC -O0 -g -Wall -Wno-unused -fno-pie -no-pie -o t1_ksymtab     t1_ksymtab.c
$CC -O0 -g -Wall -Wno-unused -fno-pie -no-pie -o t2_reloc       t2_reloc.c
$CC -O0 -g -Wall -Wno-unused -fno-pie -no-pie -o t3_modinfo     t3_modinfo.c
$CC -O0 -g -Wall -Wno-unused -fno-pie -no-pie -o t4_this_module t4_this_module.c
$CC -O0 -g -Wall -Wno-unused -fno-pie -no-pie -o t5_elf_loader  t5_elf_loader.c
# 被加载的“模块”编译成可重定位 .o（-c，不链接）
$CC -O0 -g -c -o t5_module.o t5_module.c

echo
echo "############ T1: EXPORT_SYMBOL 段收集 + bsearch ############"
./t1_ksymtab

echo
echo "############ T2: 重定位公式 S+A / S+A-P ############"
./t2_reloc

echo
echo "############ T3: .modinfo 段 + vermagic ############"
./t3_modinfo

echo
echo "############ T4: __this_module 指定初始化 + alias ############"
./t4_this_module

echo
echo "############ T5: 真实 ELF 重定位加载器 ############"
# 展示 .o 的重定位条目，证明未决符号
readelf -r t5_module.o 2>/dev/null | head -15 || true
echo "----"
./t5_elf_loader t5_module.o

echo
echo "############ 全部完成 ############"
