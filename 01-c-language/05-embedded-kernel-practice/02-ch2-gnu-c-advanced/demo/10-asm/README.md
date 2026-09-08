# demo 10-asm：内联汇编全量实测

13 个测试 + 1 个 .S 文件，覆盖 3.6 全部要点。一键复现：

```bash
# WSL x86-64, gcc 13.3 / clang 18.1.3
bash run.sh
```

## 测试矩阵

| 文件 | 主题 | 关键发现 |
|------|------|----------|
| t1_basic_vs_ext | basic asm vs extended asm | `%` 在 basic 是字面量，在 extended 是操作数引用 |
| t2_named | 命名约束 `%[name]` vs 位置 `%0` | gcc/clang 都容忍混用（不报错但行为未定义） |
| t3_output_constraints | `=r`/`=m`/`+r` | `+r` 原地修改不加 load/store |
| t4_input_constraints | `r`/`m`/`i`/`n`/`g`/`0` | clang 不接受 `"0"` 配 `"+r"` |
| t5_earlyclobber | `&` earlyclobber | `=&r` 输出不与输入别名；`=r` 可能别名 → corrupt |
| t6_clobber | `"memory"`/寄存器 clobber | 漏 clobber 后果不可预测 |
| t7_volatile | `volatile` 语义 | -O2 下无 volatile → asm 被删；有 volatile → 保留 |
| t8_opt_levels | -O0 vs -O2 | demo_bug: gcc-8 clang-10（漏 rax clobber） |
| t9_asm_goto | `asm goto` | `bt/jb` 成功生成条件跳转到 C 标签 |
| t10_bad_constraints | 约束写错 | movl+64位→汇编错误；`=r`不写→垃圾值 |
| t11_abi | x86-64 SysV ABI | 六参数从 rdi/rsi/rdx/rcx/r8/r9 加载 |
| t12_asm_calls_c.S | .S 调 C | `call c_add` 按 ABI 传参 |
| t13_kernel_patterns | 内核实例 | rdtsc/mfence/xchg/cmpxchg/cpuid |

## ARM vs x86 注意

ARM 的 `add` 是三操作数（`add dst, s1, s2`），x86 的 `add` 是两操作数（`add src, dst`）。
在 x86 上写 ARM 三操作数语法会生成非法指令 → `Illegal instruction`。
demo 里的 `add` 都用 `mov + add` 两步模拟。

对应 [3.6 C语言和汇编语言混合编程](../../3.6-mixed-programming/3.6-C语言和汇编语言混合编程.md)。
