# 10.7 易错点清单

> 来源：§10.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 7 大易错点

### 1. 忘记 volatile

**症状**：有副作用的汇编被优化掉（读寄存器返回旧值、屏障不生效）。

**原因**：`asm(...)` 如果输出操作数未被使用，编译器认为没有副作用可能删除整段汇编。

```c
/* ✗ 缺少 volatile，可能被删 */
asm("mrs %0, cntvct_el0" : "=r"(t));
asm("nop");
asm("dmb ish");

/* ✓ 加 volatile */
asm volatile("mrs %0, cntvct_el0" : "=r"(t));
asm volatile("nop");
asm volatile("dmb ish" ::: "memory");
```

**规则**：读系统寄存器、屏障、I/O 操作、任何有副作用（不只计算）的汇编 **必须加 volatile**。

### 2. 忘记 "memory" clobber

**症状**：内存访问被编译器重排，导致数据不一致。

**原因**：编译器不知道汇编修改了内存，把汇编前后的内存访问重排。

```c
/* ✗ 缺少 "memory" */
*flag = 1;
asm volatile("nop");        /* 编译器可能把 *flag=1 重排到 nop 后 */
data_ready = 1;

/* ✓ 加 "memory" */
*flag = 1;
asm volatile("nop" ::: "memory");   /* 编译器屏障，阻止重排 */
data_ready = 1;
```

### 3. 约束选错

**症状**：编译器生成错误代码、结果不对、偶发 crash。

**常见错误**：

```c
/* ✗ 用 "r" 但汇编需要立即数 */
asm("add %0, %1, #0x1000" : "=r"(y) : "r"(x));  /* #0x1000 是立即数不是变量 */

/* ✓ 用 "I" 约束 */
asm("add %0, %1, %2" : "=r"(y) : "r"(x), "I"(0x1000));

/* ✗ 用 "=r" 但汇编先读再写 */
asm("add %0, %0, #1" : "=r"(x));  /* 应该用 "+r" */

/* ✓ 用 "+r" */
asm("add %0, %0, #1" : "+r"(x));
```

| 常见错误 | 正确做法 |
|---------|---------|
| `=r` 但汇编先读旧值 | 用 `+r` |
| `r` 传立即数 | 用 `I`/`i` |
| `r` 传内存地址 | 用 `m` 或 `Q` |
| 忘 `&` 导致输入输出共用寄存器 | 用 `=&r` |

### 4. early clobber 遗漏

**症状**：输入输出寄存器冲突，结果偶发错误（取决于编译器寄存器分配）。

**原因**：输出寄存器在输入使用完前被写，编译器复用寄存器导致输入被覆盖。

```c
/* ✗ 危险：mov 写 %0 后 add 还读 %1，编译器可能让 %0=%1 */
asm(
    "mov %0, %1\n"
    "add %0, %0, %2\n"
    : "=r"(result) : "r"(a), "r"(b)
);

/* ✓ 用 & 强制分离 */
asm(
    "mov %0, %1\n"
    "add %0, %0, %2\n"
    : "=&r"(result) : "r"(a), "r"(b)
);
```

**判断标准**：输出操作数在**所有输入操作数最后一次使用之前**被写入 → 需要 `&`。

### 5. 指令选择与约束不匹配

**症状**：链接错误或运行时非法指令。

```c
/* ✗ 32 位变量用 64 位操作 */
int x = 42;
asm("mov %x0, %x1" : "=r"(x) : "r"(42));  /* %x0 是 64 位 */

/* ✓ 32 位变量用 %w0 */
asm("mov %w0, %w1" : "=r"(x) : "r"(42));  /* %w0 是 32 位 */
```

| 变量类型 | 操作数修饰符 | 寄存器 |
|---------|------------|--------|
| int/float (32 位) | `%w0` | W0-W30 |
| long/double (64 位) | `%x0` | X0-X30 |
| 向量 (128 位) | `%v0` 或 `%q0` | V0-V31 |

### 6. 忘记 "cc" clobber

**症状**：条件标志被破坏，后续 C 条件判断错误。

**原因**：汇编中 CMP/ADDS 修改了 NZCV 但没声明，编译器以为标志不变。

```c
/* ✗ 缺少 "cc" */
asm volatile(
    "cmp %0, %1\n"
    : "+r"(x) : "r"(y)
);                          /* CMP 改了 NZCV 但没声明 */
if (x == y) { ... }         /* C 的 if 可能用旧的 NZCV → bug */

/* ✓ 加 "cc" */
asm volatile(
    "cmp %0, %1\n"
    : "+r"(x) : "r"(y)
    : "cc"                   /* 声明 NZCV 被修改 */
);
```

### 7. 多条指令不分行

**症状**：汇编器报错 `Error: unrecognized opcode`。

**原因**：多条指令挤在一行，GNU as 按行解析无法识别。

```c
/* ✗ 多条指令在一行 */
asm("mov x0, #1 mov x1, #2");  /* as 报错 */

/* ✓ 用 \n\t 分隔 */
asm("mov x0, #1\n\t"
    "mov x1, #2\n\t");
```

## 易错点速查表

| # | 易错点 | 症状 | 一句话修复 |
|---|--------|------|-----------|
| 1 | 忘 volatile | 汇编被删 | 有副作用就加 volatile |
| 2 | 忘 "memory" | 内存重排 | 有内存操作加 "memory" |
| 3 | 约束选错 | 生成错误代码 | 先读再写用 +r，立即数用 I |
| 4 | 忘 & | 输入输出冲突 | 输出先于输入用完用 =&r |
| 5 | 指令/约束不匹配 | 非法指令 | 32 位用 %w, 64 位用 %x |
| 6 | 忘 "cc" | 标志被破坏 | CMP/ADDS 后加 "cc" |
| 7 | 不分行 | as 报错 | 用 \n\t 分隔 |

## 自测题

1. 以下代码有什么问题？
```c
u64 read_time(void) {
    u64 t;
    asm("mrs %0, cntvct_el0" : "=r"(t));
    return t;
}
```
<details><summary>答案</summary>
缺少 `volatile`。读取时间戳有副作用（每次值不同），如果 t 未被使用编译器可能删掉整段汇编。应改为 `asm volatile("mrs %0, cntvct_el0" : "=r"(t));`。
</details>

2. 以下代码为什么可能产生错误结果？
```c
asm volatile(
    "mov %0, %1\n"
    "add %0, %0, #1"
    : "=r"(result) : "r"(input)
);
```
<details><summary>答案</summary>
缺少 `&`（early clobber）。`mov %0, %1` 先写 %0，然后 `add %0, %0, #1` 读 %0（此时已经是 mov 后的值）。但问题在于编译器可能让 %0 和 %1 共用寄存器 → mov 后 %1（输入）被覆盖。如果后续还有指令读 %1 就会读到错误值。应改为 `"=&r"(result)`。
</details>

3. 内联汇编中忘记了 `"memory"` clobber 会怎样？
<details><summary>答案</summary>
编译器可能把汇编前后的内存访问重排到另一侧。例如 `*ptr = 1; asm("nop"); ready = 1;` 如果没有 "memory"，编译器可能把 `ready = 1` 重排到 `asm` 之前或把 `*ptr = 1` 重排到之后 → 数据不一致。有内存副作用的汇编必须加 "memory"。
</details>

4. 以下代码哪里有问题？
```c
int flag = 1;
asm volatile(
    "tst %0, #1\n"
    "cset %0, eq\n"
    : "+r"(flag)
);
```
<details><summary>答案</summary>
缺少 `"cc"` clobber。TST 修改了 NZCV 条件标志，如果不声明 "cc"，编译器可能不知道标志被改 → 后续依赖条件标志的 C 代码可能用旧的 NZCV。应改为 `: "+r"(flag) : : "cc"`。
</details>

5. 以下代码编译后可能生成什么问题？
```c
asm(
    "mov x0, #1 mov x1, #2"
);
```
<details><summary>答案</summary>
两条指令挤在一行没有分隔符，GNU as 会尝试把 `mov x0, #1 mov x1, #2` 当作一条指令解析 → 报错 `unrecognized opcode`。应该用 `\n\t` 或 `;` 分隔：`"mov x0, #1\n\t" "mov x1, #2\n\t"`。
</details>

## 参考与延伸

- 原书 §10.7
- [10.2 约束字符](02-constraints.md)
- [10.4 clobber 列表](04-clobber.md)
