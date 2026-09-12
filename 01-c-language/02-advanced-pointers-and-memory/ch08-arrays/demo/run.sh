#!/usr/bin/env bash
# 8.1 一维数组 · 实测脚本（数组 vs 指针 + `= {0}` 语义）
# 环境：WSL Ubuntu，gcc 13.3 / clang 18.1.3
# 用法：bash run.sh
set -u
CC=${CC:-gcc}
cd "$(dirname "$0")"

hr() { printf '\n==================== %s ====================\n' "$1"; }

hr "T1: 数组 vs 指针 · 逐项对照"
$CC -std=c11 -O0 -Wall -Wno-unused -fno-pie -no-pie -o t1_array_vs_pointer t1_array_vs_pointer.c \
  && ./t1_array_vs_pointer

hr "T1 附 a: 符号表大小（400 字节数组 vs 8 字节指针）"
cat > /tmp/t1_sym.c <<'EOF'
int g_arr[100];     /* 100 * 4 = 400 字节 */
int *g_ptr;         /* 8 字节 */
EOF
$CC -c /tmp/t1_sym.c -o /tmp/t1_sym.o 2>/dev/null
echo "  -- nm --print-size --size-sort --radix=d --"
nm --print-size --size-sort --radix=d /tmp/t1_sym.o 2>/dev/null | grep -E 'g_arr|g_ptr'

hr "T1 附 b: 数组名不可赋值 / 不可自增（编译器原文）"
cat > /tmp/t1_err.c <<'EOF'
int main(void) { int a[10], *p; a = p; return 0; }
EOF
echo "  -- a = p; --"
$CC -std=c11 -c /tmp/t1_err.c -o /dev/null 2>&1 | head -3
cat > /tmp/t1_err2.c <<'EOF'
int main(void) { int a[10]; a++; return 0; }
EOF
echo "  -- a++; --"
$CC -std=c11 -c /tmp/t1_err2.c -o /dev/null 2>&1 | head -3
cat > /tmp/t1_err3.c <<'EOF'
int main(void) { int a[10]; &a = 0; return 0; }
EOF
echo "  -- &a = 0; --"
$CC -std=c11 -c /tmp/t1_err3.c -o /dev/null 2>&1 | head -3

hr "T2: \`= {0}\` 语义 —— 全 0 的机制"
$CC -std=c11 -O0 -Wall -Wno-unused -fno-pie -no-pie -o t2_zero_init t2_zero_init.c \
  && ./t2_zero_init

hr "T2 附 a: 段布局（.data vs .bss）"
echo "  -- size --"
size t2_zero_init 2>/dev/null | sed 's/^/  /'
echo "  -- nm --print-size（B=.bss, D=.data, b=static .bss）--"
nm --print-size --radix=d t2_zero_init 2>/dev/null \
  | grep -E 'g_zero|g_one|g_none|g_static' | sed 's/^/  /'
echo "  注：g_zero 写了 = {0} 却落在 .bss —— 全零的 .data 会被链接器"
echo "      收回 .bss（反正加载时都清零），所以显式 = {0} 不占文件体积。"

hr "T2 附 b: \`= {0}\` 编译成什么"
echo "  -- -O0 下 int a[10] = {0}（应见 xor/rep stos/memset）--"
cat > /tmp/t2_asm.c <<'EOF'
void f(void) { int a[10] = {0}; (void)a; }
EOF
$CC -std=c11 -O0 -S -o - /tmp/t2_asm.c 2>/dev/null \
  | grep -E '^\s+(mov|xor|rep|call|vmov)' | head -8 | sed 's/^/  /'
echo "  -- -O2 下同函数（常量折叠 + DSE 后可能整段消失）--"
$CC -std=c11 -O2 -S -o - /tmp/t2_asm.c 2>/dev/null \
  | sed -n '/^f:/,/ret/p' | head -10 | sed 's/^/  /'

hr "T3: 清零的代价（HFT 判据）"
$CC -std=c11 -O2 -Wall -Wno-unused -fno-pie -no-pie -o t3_clear_cost t3_clear_cost.c \
  && ./t3_clear_cost

hr "T3 附: DSE 前后汇编对比（清零是否被消除）"
cat > /tmp/t3_dse.c <<'EOF'
#include <string.h>
void f(char *buf, unsigned n) { char t[4096] = {0}; memset(t, 'A', sizeof t); buf[0] = t[n % 4096]; }
void g(char *buf, unsigned n) { char t[4096] = {0}; memcpy(t, buf, n % 4096); buf[0] = t[0]; }
EOF
echo "  -- f(): 清零后 + 完整覆盖 -> -O2 应消除清零 --"
$CC -O2 -S -o - /tmp/t3_dse.c 2>/dev/null | sed -n '/^f:/,/ret/p' \
  | grep -cE 'memset|rep stos' | sed 's/^/    memset\/rep 指令条数: /'
echo "  -- g(): 清零后 + 部分覆盖 -> 清零保留 --"
$CC -O2 -S -o - /tmp/t3_dse.c 2>/dev/null | sed -n '/^g:/,/ret/p' \
  | grep -cE 'memset|rep stos' | sed 's/^/    memset\/rep 指令条数: /'

hr "T4: 数组作函数参数 · 三种写法等价"
$CC -std=c11 -O0 -Wall -Wno-unused -Wno-sizeof-array-argument -fno-pie -no-pie \
   -o t4_param_decay t4_param_decay.c && ./t4_param_decay

echo "  -- 附: 函数内对形参用 sizeof，gcc/clang 的警告原文 --"
$CC -std=c11 -O0 -Wall -c t4_param_decay.c -o /dev/null 2>&1 \
  | grep -E 'sizeof.*array function parameter|declared here' | head -4 | sed 's/^/    /'

hr "双编译器验证（gcc + clang）"
for f in t1_array_vs_pointer t2_zero_init t3_clear_cost t4_param_decay; do
  if clang -std=c11 -O2 -Wall -Wno-unused -Wno-unused-but-set-variable -fno-pie -no-pie \
       -c "$f.c" -o "/tmp/c_$f.o" 2>/tmp/c_err_$f; then
    echo "  clang OK   : $f.c"
  else
    echo "  clang FAIL : $f.c"
    head -3 "/tmp/c_err_$f" | sed 's/^/      /'
  fi
done

hr "完成"
echo "  gcc: $($CC --version | head -1)"
echo "  clang: $(clang --version | head -1)"
