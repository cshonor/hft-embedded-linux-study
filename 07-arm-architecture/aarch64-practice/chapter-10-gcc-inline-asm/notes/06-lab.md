# 10.6 实验要点

> 来源：§10.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 实验列表

| 实验 | 内容 | 关键命令 |
|------|------|---------|
| 10-1 | 基本内联汇编（加减法） | `gcc -O2 -S` |
| 10-2 | 系统寄存器读写 | `mrs`/`msr` |
| 10-3 | memset 实现 | `objdump -d` 对比 |
| 10-4 | 原子操作 | `ldxr`/`stxr` |
| 10-5 | asm goto | static_branch |

---

## 实验 10-1：基本内联汇编

### 目标

编写加法内联汇编，验证 GCC 生成的指令正确性。

### 代码

```c
/* add_inline.c */
#include <stdio.h>

static inline int my_add(int a, int b)
{
    int result;
    asm volatile(
        "add %w0, %w1, %w2"
        : "=r"(result)
        : "r"(a), "r"(b)
    );
    return result;
}

int main(void)
{
    printf("3 + 4 = %d\n", my_add(3, 4));
    return 0;
}
```

### 操作步骤

```bash
# 在 WSL 中用 aarch64 交叉编译
aarch64-linux-gnu-gcc -O2 -c add_inline.c -o add_inline.o

# 反汇编查看实际指令
aarch64-linux-gnu-objdump -d add_inline.o

# 期望看到：
#   add w0, w1, w2     ← 内联汇编的 ADD 指令

# 对比不同优化级别
aarch64-linux-gnu-gcc -O0 -S add_inline.c -o add_O0.s
aarch64-linux-gnu-gcc -O2 -S add_inline.c -o add_O2.s
diff add_O0.s add_O2.s    # -O2 可能把整个函数内联到 main
```

### 观察要点

| 检查项 | -O0 | -O2 |
|------|-----|-----|
| my_add 是否被内联 | ✗（函数调用） | ✓（内联到 main） |
| ADD 指令出现 | ✓ | ✓ |
| 寄存器分配 | 固定 | 编译器优化 |

---

## 实验 10-2：系统寄存器读写

### 目标

读取 CurrentEL 和 cntvct_el0，理解系统寄存器访问。

### 代码

```c
/* sysreg_test.c */
#include <stdio.h>

#define read_sysreg(reg) ({           \
    unsigned long val;                 \
    asm volatile("mrs %0, " #reg      \
        : "=r"(val));                  \
    val;                               \
})

int main(void)
{
    unsigned long el = read_sysreg(CurrentEL);
    unsigned long tsc = read_sysreg(cntvct_el0);

    printf("CurrentEL = 0x%lx (EL%ld)\n", el, el >> 2);
    printf("CNTVCT_EL0 = 0x%lx\n", tsc);

    /* 测量延迟 */
    unsigned long t1 = read_sysreg(cntvct_el0);
    for (volatile int i = 0; i < 1000; i++);
    unsigned long t2 = read_sysreg(cntvct_el0);
    printf("1000 iterations took %ld ticks\n", t2 - t1);

    return 0;
}
```

### 操作步骤

```bash
# 编译
aarch64-linux-gnu-gcc -O2 -o sysreg_test sysreg_test.c -static

# QEMU 运行
qemu-aarch64 ./sysreg_test

# 期望输出：
#   CurrentEL = 0x1 (EL1)     ← QEMU user mode 模拟 EL1
#   CNTVCT_EL0 = 0x1234...
#   1000 iterations took XX ticks
```

---

## 实验 10-3：memset 实现

### 目标

用内联汇编实现 memset，与 glibc 性能对比。

### 代码

```c
/* my_memset.c */
#include <stdio.h>
#include <string.h>
#include <time.h>

static inline void *my_memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    asm volatile(
        "1: cbz %2, 2f\n"
        "   strb %w1, [%0], #1\n"    /* 写 1 字节 */
        "   sub %2, %2, #1\n"        /* n-- */
        "   b 1b\n"
        "2:\n"
        : "+r"(p), "+r"(c), "+r"(n)
        :
        : "memory"
    );
    return s;
}

int main(void)
{
    char buf1[1024], buf2[1024];

    /* 功能正确性测试 */
    my_memset(buf1, 0xAA, 1024);
    memset(buf2, 0xAA, 1024);
    printf("Correct: %s\n", memcmp(buf1, buf2, 1024) == 0 ? "YES" : "NO");

    /* 性能对比 */
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    for (int i = 0; i < 100000; i++)
        my_memset(buf1, 0, 1024);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    printf("my_memset: %ld ns\n", (t2.tv_sec - t1.tv_sec) * 1000000000L + t2.tv_nsec - t1.tv_nsec);

    return 0;
}
```

### 操作步骤

```bash
aarch64-linux-gnu-gcc -O2 -o memset_test my_memset.c -static
qemu-aarch64 ./memset_test

# 反汇编对比
aarch64-linux-gnu-objdump -d memset_test | grep -A 20 "my_memset"
```

---

> **实验 10-4（原子操作）和 10-5（asm goto）** 详见 [06b-lab-advanced.md](06b-lab-advanced.md)

## 参考与延伸

- 原书 §10.6
- [→ 进阶实验（原子操作 + asm goto）](06b-lab-advanced.md)
- [10.7 易错点清单](07-pitfalls.md)
- [Ch22 NEON 优化](../../chapter-22-fp-neon/notes/section-0-本章完整概述.md)
