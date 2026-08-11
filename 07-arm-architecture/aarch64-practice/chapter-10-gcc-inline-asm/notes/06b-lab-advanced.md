# 10.6b 实验要点 — 进阶（原子操作 + asm goto）

> 来源：§10.6 · 精读 · [← 基础实验](06-lab.md) · [章总览](section-0-本章完整概述.md)

## 实验列表

| 实验 | 内容 | 关键命令 |
|------|------|---------|
| 10-4 | 原子操作 | `ldxr`/`stxr` |
| 10-5 | asm goto | static_branch |

基础实验（10-1 ~ 10-3）见 [06-lab.md](06-lab.md)。

---

## 实验 10-4：原子操作

### 目标

实现原子加法，验证多线程安全。

### 代码

```c
/* atomic_add.c */
#include <stdio.h>
#include <pthread.h>

static inline void atomic_add(int *ptr, int val)
{
    int tmp;
    asm volatile(
        "1: ldxr %w0, [%1]\n"
        "   add %w0, %w0, %w2\n"
        "   stxr %w3, %w0, [%1]\n"
        "   cbnz %w3, 1b\n"
        : "=&r"(tmp)
        : "r"(ptr), "r"(val), "r"(tmp)
        : "cc", "memory"
    );
}

static int counter = 0;

void *worker(void *arg)
{
    for (int i = 0; i < 100000; i++)
        atomic_add(&counter, 1);
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("counter = %d (expected 200000)\n", counter);
    return 0;
}
```

### 操作步骤

```bash
aarch64-linux-gnu-gcc -O2 -pthread -o atomic_test atomic_add.c -static
qemu-aarch64 ./atomic_test
# 期望：counter = 200000
```

---

## 实验 10-5：asm goto

### 目标

用 asm goto 实现条件分支，对比与普通 if 的开销。

### 代码

```c
/* goto_test.c */
#include <stdio.h>

int check_bit(unsigned int x)
{
    asm goto(
        "tbz %w0, #0, %l[zero]\n"
        : : "r"(x) : : zero
    );
    return 1;     /* bit0 = 1 */

zero:
    return 0;     /* bit0 = 0 */
}

int main(void)
{
    printf("check_bit(0) = %d\n", check_bit(0));
    printf("check_bit(1) = %d\n", check_bit(1));
    printf("check_bit(3) = %d\n", check_bit(3));
    return 0;
}
```

### 操作步骤

```bash
aarch64-linux-gnu-gcc -O2 -o goto_test goto_test.c -static
qemu-aarch64 ./goto_test

# 反汇编查看：应该是 TBZ 指令
aarch64-linux-gnu-objdump -d goto_test | grep -A 5 "check_bit"
```

---

## 自测题

1. 如何验证内联汇编生成的指令正确？
<details><summary>答案</summary>
（1）`objdump -d` 反汇编查看实际指令（2）GDB 断点 + 单步执行验证（3）对比 `-O0` 和 `-O2` 的输出看优化行为（4）功能测试：编写测试用例验证输出正确性。最可靠的是反汇编 + 功能测试结合。
</details>

2. 内联汇编 memset 和 glibc memset 性能差距来自哪里？
<details><summary>答案</summary>
glibc memset 用 NEON 向量指令（STP 一次写 128 字节）+ 循环展开 + 按对齐优化。简单内联用 STRB（1 字节）或 STR（4 字节）循环。差距可达 10-100 倍。生产环境用 glibc 的 memset，内联版只用于裸机环境无 libc 的场景。
</details>

3. 实验 10-4 中如果去掉 `"memory"` clobber 会怎样？
<details><summary>答案</summary>
编译器可能把 counter 的读取缓存到寄存器中，不每次从内存重新加载 → LDXR 读到的可能是寄存器中的旧值而非最新内存值 → 原子操作失效。`"memory"` 强制每次从内存重新加载。
</details>

4. 实验 10-1 中 `-O2` 下 my_add 的输出和 `-O0` 有什么不同？
<details><summary>答案</summary>
`-O2` 下 `static inline` 函数被内联到调用点，my_add 不再是独立函数。`3+4=7` 可能在编译期直接算出（常量折叠），不生成任何 ADD 指令。`-O0` 下不内联，my_add 是真实函数调用，有完整的函数序言/结语。
</details>

5. 实验 10-5 中 asm goto 生成的 TBZ 指令和 `if (x & 1)` 编译后的指令有什么区别？
<details><summary>答案</summary>
功能相同（都是检查 bit0），但 asm goto 的 TBZ 指令由用户直接控制。编译器生成的可能也是 TBZ（如果优化好）或 TST+B.NE。asm goto 的优势在于可以精确控制指令选择和跳转目标，配合 static_branch 还能实现 NOP patching。
</details>

---

## 参考与延伸

- 原书 §10.6
- [← 基础实验](06-lab.md)
- [10.7 易错点清单](07-pitfalls.md)
- [Ch22 NEON 优化](../../chapter-22-fp-neon/notes/section-0-本章完整概述.md)
