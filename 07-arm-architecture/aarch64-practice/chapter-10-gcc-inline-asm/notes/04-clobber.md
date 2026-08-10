# 10.4 clobber 列表

> 来源：§10.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

clobber 列表声明内联汇编修改了哪些不在输入/输出操作数中的资源：寄存器、条件标志、内存。

## clobber 的作用

GCC 需要知道哪些寄存器/资源被汇编修改，才能正确保存和恢复：

```
如果没有 clobber 声明：
  1. 编译器把变量 a 分配在 X0
  2. 内联汇编修改了 X0（但没声明）
  3. 汇编后 a 的值被破坏 → 难以调试的 bug

正确做法：
  1. 在 clobber 中声明 "x0"
  2. 编译器知道 X0 被修改 → 不把变量分配在 X0
  3. 或在使用前保存 X0、使用后恢复
```

## clobber 类型速查

| clobber | 含义 | 何时使用 |
|---------|------|---------|
| `"memory"` | 汇编修改内存（编译器屏障） | 有内存副作用：LDR/STR/原子操作 |
| `"cc"` | 修改 NZCV 条件标志 | CMP/CMP/TST/ADDS/SUBS |
| `"x0"`-`"x30"` | 修改指定寄存器 | 汇编直接使用寄存器 |
| `"v0"`-`"v31"` | 修改 SIMD/浮点寄存器 | 浮点/SIMD 操作 |
| `"nzcv"` | 等价于 "cc"（某些版本） | 同 "cc" |

## "memory" clobber 详解

### 编译器屏障

```c
/* ✗ 危险：编译器可能重排 */
*ptr = 1;
asm volatile("nop");   /* 编译器可能把 *ptr=1 重排到 nop 后 */
flag = 1;

/* ✓ 正确：memory clobber 阻止重排 */
*ptr = 1;
asm volatile("nop" ::: "memory");  /* 编译器屏障 */
flag = 1;                            /* 不会被重排到 nop 前 */
```

### "memory" 的行为

| 行为 | 说明 |
|------|------|
| 编译器记住的内存值失效 | 所有缓存在寄存器中的变量需重新从内存加载 |
| 阻止重排 | asm 前的内存写不会移到 asm 后；asm 后的读不会移到 asm 前 |
| 不生成 CPU 指令 | 只是编译器行为，不产生 DMB/DSB |

### 对比：编译器屏障 vs CPU 屏障

```c
/* 编译器屏障：不生成 CPU 指令 */
#define barrier() asm volatile("" ::: "memory")

/* CPU 内存屏障：生成 DMB 指令 */
#define dmb(opt) asm volatile("dmb " #opt ::: "memory")
```

| 层面 | 编译器屏障 | CPU 屏障 |
|------|----------|---------|
| 作用对象 | 编译器 | CPU 硬件 |
| 阻止 | 编译器重排内存访问 | CPU 乱序执行 |
| 指令 | 无（编译期行为） | DMB/DSB/ISB |
| 场景 | 单核内序保证 | 多核间序保证 |

## "cc" clobber 详解

```c
/* 汇编中用了 CMP/ADDS 等修改标志的指令 */
asm volatile(
    "cmp %0, %1\n"      /* CMP 修改 NZCV */
    "cset %0, eq\n"     /* 读取 Z 标志 */
    : "+r"(result)
    : "r"(value)
    : "cc"               /* 必须声明！ */
);
```

### 什么时候需要 "cc"

| 指令 | 是否修改 NZCV | 需要 "cc" |
|------|-------------|----------|
| CMP | ✓ | ✓ |
| CMN | ✓ | ✓ |
| TST | ✓ | ✓ |
| ADDS | ✓ | ✓ |
| SUBS | ✓ | ✓ |
| ADD | ✗ | ✗ |
| MOV | ✗ | ✗ |
| LDR/STR | ✗ | ✗ |
| CSEL/CSET | 读取但不修改 | ✗ |

## 寄存器 clobber

```c
/* 汇编直接使用 X0 但不在输出操作数中 */
asm volatile(
    "mov x0, #1\n\t"
    "svc #0\n\t"          /* 系统调用：X8=系统调用号, X0=参数 */
    : "=r"(ret)
    : "r"(nr)
    : "x0", "memory"      /* 声明 X0 被修改 */
);
```

### AArch64 寄存器 clobber 注意事项

| 寄存器 | clobber 声明 | 说明 |
|--------|-------------|------|
| X0-X18 | 需声明 | caller-saved，编译器不保存 |
| X19-X30 | 通常不需要 | callee-saved，函数自动保存 |
| SP | 特殊 | 通常不直接修改 |
| V0-V7 | 需声明 | caller-saved 浮点寄存器 |
| V8-V15 | 通常不需要 | callee-saved（下半部分） |

## 常见 clobber 模式

| 模式 | clobber | 典型场景 |
|------|---------|---------|
| `::: "memory"` | 仅内存屏障 | barrier() 编译器屏障 |
| `::: "cc"` | 仅条件标志 | CMP 后 CSET |
| `::: "cc", "memory"` | 标志 + 内存 | 原子操作（CMP + STXR） |
| `::: "x0", "x1", "memory"` | 指定寄存器 | 系统调用 |

## 过多 clobber 的代价

```c
/* ✗ 过多的 "memory" clobber */
asm volatile("nop" ::: "memory");  /* 每次都让所有寄存器变量失效 */

/* 在热循环中频繁使用会导致：
   1. 每次都要从内存重新加载变量（不缓存到寄存器）
   2. 编译器优化空间大幅缩小
   3. 性能下降 */
```

**原则**：只在确实有内存副作用的汇编上加 `"memory"`，纯计算的汇编不加。

## HFT 关联

- **`"memory"` 是编译器屏障** → 阻止编译器重排内存访问，但不生成 CPU 指令
- **过多 `"memory"` 限制编译器优化** → 热循环中每次都刷新寄存器缓存
- **`"cc"` 在原子操作中必需** → CMP/ADDS 修改标志后 CSET 读取
- **精确的 clobber 声明** → 声明越少编译器优化空间越大，但漏声明会导致 bug

## 自测题

1. 什么时候需要 `"cc"` clobber？
<details><summary>答案</summary>
汇编修改了 NZCV 条件标志时（如 ADDS/SUBS/CMP/CMN/TST）。告诉编译器标志被修改，后续依赖条件标志的代码（如 if 判断、CSEL）需重新评估。ADD/MOV/LDR 不修改标志，不需要 "cc"。
</details>

2. `"memory"` clobber 等价于什么屏障？
<details><summary>答案</summary>
编译器屏障（compiler barrier），等价于内核的 `barrier()` 宏。阻止编译器把内存访问重排到 asm 另一侧，并让寄存器中缓存的变量值失效（需重新加载）。但不生成 CPU 内存屏障指令（DMB/DSB）。
</details>

3. 如果汇编修改了 X0 但没在 clobber 中声明会怎样？
<details><summary>答案</summary>
编译器可能把 C 变量分配在 X0 中。汇编修改 X0 后 C 变量值被破坏 → 难以调试的 bug（值在某些情况下对、某些情况下错，取决于编译器寄存器分配）。正确做法是在 clobber 中声明 "x0"。
</details>

4. `"memory"` clobber 和 `dmb ish` 有什么区别？
<details><summary>答案</summary>
`"memory"` 是编译器屏障：只阻止编译器重排内存访问（编译期行为），不生成任何 CPU 指令。`dmb ish` 是 CPU 屏障：生成 DMB 指令，阻止 CPU 乱序执行（运行时硬件行为）。多核场景需要 CPU 屏障；单核防编译器优化只需 "memory"。
</details>

5. 以下汇编需要哪些 clobber？
```c
asm volatile(
    "ldxr %w0, [%1]\n"
    "cmp %w0, %w2\n"
    "stxr w3, %w2, [%1]\n"
    : "=&r"(old) : "r"(ptr), "r"(new) : ???
);
```
<details><summary>答案</summary>
需要 `"cc", "memory"`。"cc" 因为 CMP 修改 NZCV。"memory" 因为 LDXR/STXR 有内存副作用（读/写 [ptr]）。另外如果 STXR 的结果写到了 w3 但 w3 不在输出操作数中，还需要声明 "w3" 或把 w3 加入输出操作数。
</details>

## 参考与延伸

- 原书 §10.4
- [10.1 基本语法](01-basic-syntax.md)
- [Ch18 编译器屏障 vs CPU 屏障](../../chapter-18-memory-barriers/notes/section-0-本章完整概述.md)
