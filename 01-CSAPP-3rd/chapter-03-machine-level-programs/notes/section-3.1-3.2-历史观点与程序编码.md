## 3.1–3.2 历史观点与程序编码

### 3.1 历史观点

- **ISA 演进：** 8086 → 32 位 x86 → **x86-64 (AMD64)** — 本书以 **x86-64 Linux** 为主
- **复杂指令集 (CISC)** — 指令长度可变、丰富寻址模式；与 ARM RISC 对比心里有数即可
- **HFT 现实：** 共置服务器几乎全是 **x86-64**；ARM 云实例增多但低延迟栈仍以 Intel/AMD 为主

### 3.2.1 机器级代码

- **机器码** = 字节序列；**汇编** = 人类可读的助记符
- **编译器** 把 C 翻译成汇编，汇编器成 `.o`，链接器合并
- 同一 C 代码，`-O0` vs `-O3` 汇编 **差异巨大** — profile 要看 **Release 构建**

```bash
gcc -Og -S -fno-verbose-asm foo.c -o foo.s   # 学习用，适度优化
gcc -O3 -S foo.c
objdump -d ./a.out | less
```

### 3.2.2 代码示例

典型模式：函数序言 **push %rbp; mov %rsp,%rbp; sub $N,%rsp** 分配栈帧（现代编译器也常用 **不帧指针** 的 `%rsp` 相对寻址）。

### 3.2.3 关于格式的注解

| 风格 | 特点 |
|------|------|
| **AT&T**（gcc -S 默认） | `movq %rax, %rbx` — 源在左、目标在右；寄存器 `%` 前缀 |
| **Intel**（IDA、部分文档） | `mov rbx, rax` — 目标在左 |

**操作数后缀：** `b/w/l/q` = 1/2/4/8 字节（如 `movl` = 4 字节）

**HFT：** `perf annotate`、编译器输出多为 AT&T；对照 Intel 文档时注意 **操作数顺序相反**。

→ 流水线与性能：[Ch 4](../../chapter-04-processor-architecture/)

---

← [本章导读](../README.md)
