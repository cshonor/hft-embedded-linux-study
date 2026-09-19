#!/bin/bash
# 运行 14-packed 全部 demo
# 用法: bash run.sh
set -e

CC="${CC:-gcc}"

echo "========================================"
echo " 14-packed: 协议结构体 packed 实践"
echo " 编译器: $CC ($( $CC --version | head -1 ))"
echo "========================================"
echo ""

# T1: 布局对比 (-O0 看真实布局)
echo "======== T1: 协议结构体真实布局 ========"
$CC -O0 -Wall -Wno-unused -fno-pie -no-pie -o t1_layout t1_layout.c
./t1_layout
echo ""

# T2: 位域字节序
echo "======== T2: 位域字节序 ========"
$CC -O0 -Wall -Wno-unused -fno-pie -no-pie -o t2_bitfield t2_bitfield.c
./t2_bitfield
echo ""

# T3: 未对齐访问性能 (-O2 热路径)
echo "======== T3: packed 未对齐访问性能 ========"
$CC -O2 -Wall -Wno-unused -fno-pie -no-pie -o t3_unaligned t3_unaligned.c
./t3_unaligned
echo ""

# 查看汇编指令差异
echo "======== T3 附: 汇码指令对比 ========"
$CC -O2 -S -o /tmp/t3_packed.s t3_unaligned.c 2>/dev/null
echo "  -- 未对齐 uint32_t 访问的 movzwl 指令 --"
grep -E 'movzwl|movl|movzbl' /tmp/t3_packed.s 2>/dev/null | sort -u | head -8 || true
echo ""

# T4: flag_word 整字操作
echo "======== T4: tcp_flag_word 整字 vs 位域 ========"
# 用 -O0 避免 -O2 把循环折叠成 0ms；-fno-strict-aliasing 允许 union cast
$CC -O0 -fno-strict-aliasing -Wall -Wno-unused -fno-pie -no-pie -o t4_flagword t4_flagword.c
./t4_flagword
echo ""

# T5: bswap 汇编
echo "======== T5: htonl/ntohl 实现 ========"
$CC -O2 -Wall -Wno-unused -fno-pie -no-pie -o t5_bswap t5_bswap.c
./t5_bswap
echo ""
echo "  -- bswap 汇编指令验证 --"
$CC -O2 -S -o /tmp/t5.s t5_bswap.c 2>/dev/null
# 排除 .string/.file 行，只看实际指令
bswap_count=$(grep -E '^\s+bswap' /tmp/t5.s 2>/dev/null | wc -l)
if [ "$bswap_count" -gt 0 ]; then
    echo "  ✓ 编译器生成了 $bswap_count 条 BSWAP 指令:"
    grep -E '^\s+bswap' /tmp/t5.s 2>/dev/null | head -3
else
    echo "  (未找到 BSWAP 指令，编译器可能用 mov+ror 替代)"
    grep -E '^\s+(ror|mov|shl|shr|or)' /tmp/t5.s 2>/dev/null | head -5
fi
echo ""

echo "========================================"
echo " 全部 demo 完成"
echo "========================================"
