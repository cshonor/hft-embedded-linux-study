# demo08 · `__attribute__((format))` 实测

对应 [6.8 属性声明：format](../../6.8-format/6.8-属性声明-format.md)。

这一组**不是可运行程序**，而是**编译期诊断测试**——要看的是编译器输出的 warning/error，
以及 t6/t8/t9 的反汇编。

## 怎么跑

```bash
# WSL / Linux，gcc 13.3 + clang 18
bash run.sh      # t1 ~ t6：索引语义、va_list、archetype、-Wformat 家族、作用域、零开销
bash run2.sh     # t7a~t7d：archetype 支持矩阵、-std= 方言、%n 安全、内核扩展格式符
```

t8 / t9 需要单独看反汇编：

```bash
gcc -std=gnu11 -Wall -Wformat=2 -O2 -c t8_noprintk.c -o t8.o
objdump -d t8.o                  # main 只剩 endbr64 / xor eax,eax / ret
objdump -s -j .rodata t8.o       # 空：格式串没进二进制
nm -u t8.o                       # 空：无外部引用

gcc -std=gnu11 -O2 -c t9_sideeffect.c -o t9.o
objdump -d t9.o                  # no_log 的 side() 消失，运行时门保留
```

## 文件与预期

| 文件 | 验证什么 | 关键预期 |
|------|----------|----------|
| `t1_basic.c` | 三个参数的索引语义 | 带属性报 mismatch；无属性（`log_bad`）完全静默 |
| `t2_valist.c` | `first-to-check = 0` | gcc **error**：value '2' does not refer to a variable argument list；clang 只 warning |
| `t3_archetype.c` | `%m` / `%ms` / `%'d` / `%I64d` | gcc 认 `gnu_printf`；clang 报 not supported |
| `t4_woptions.c` | `-Wformat` 家族 | truncation/overflow 需 `-O2`；`%n` 在字面量里不报 |
| `t5_scope.c` | 属性放哪儿生效 | 函数**定义**前写属性 → gcc error；函数**指针**带属性 → 有效 |
| `t6_zerooverhead.c` | 编译期门 vs 运行时门 | `-O2` 下编译期门的调用点消失，函数体仍在 |
| `t7_archetype2.c` | archetype 支持矩阵 | gcc 认 gnu_* 不认 ms_*/syslog；clang 只认基础四个 |
| `t7_dialect.c` | `-std=` 是否影响检查 | c89…gnu17 七种方言全部零警告 —— 与方言无关 |
| `t7_security.c` | `-Wformat-security` / `%n` | 只在"非常量且无参"时触发 |
| `t7_kernel.c` | `%pK` / `%pI4` / `%pS` | 两个编译器**都不报**（`%p` 后即停止解析） |
| `t8_noprintk.c` | 内核 `no_printk` 的 `if (0)` 技巧 | 检查在、代码无；属性写进宏 → 语法错误 |
| `t9_sideeffect.c` | 参数求值 | 编译期门不求值；运行时门必求值 |

## 环境

gcc (Ubuntu 13.3.0) / Ubuntu clang 18.1.3 / x86-64 / glibc 2.39。
内核源码核对引用 Linux v6.6 `include/linux/compiler_attributes.h:171`、`include/linux/printk.h:126/134/149`。
