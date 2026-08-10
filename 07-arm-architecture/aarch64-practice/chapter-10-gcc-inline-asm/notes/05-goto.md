# 10.5 goto 模板

> 来源：§10.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

`asm goto` —— GCC 扩展，允许内联汇编直接跳转到 C 代码中的标签。内核中用于 `static_branch`（静态键）实现零开销条件分支。

## 基本语法

```c
asm goto(
    "汇编指令（可跳转到 %l[label]）"
    :                         /* 无输出操作数 */
    : 输入操作数               /* 输入 */
    : clobber 列表             /* clobber */
    : C 标签列表                /* 可跳转的 C 标签 */
);
```

### 与普通 asm 的区别

| 特性 | `asm volatile` | `asm goto` |
|------|---------------|-------------|
| 输出操作数 | 有 | **无** |
| 跳转能力 | 只跳汇编内部标号 | 可跳 C 标签 |
| `%l[label]` | 不支持 | 支持 |
| GCC 版本 | 全版本 | GCC ≥ 4.5 |

### 基本示例

```c
/* 检测 bit0 是否为 0，是则跳到 C 标签 zero */
int test_bit_zero(unsigned int x)
{
    asm goto(
        "tbz %w0, #0, %l[zero]\n"   /* bit0=0 → 跳到 C 标签 zero */
        :                           /* 无输出 */
        : "r"(x)                    /* 输入：x */
        :                           /* 无 clobber */
        : zero                      /* 可跳的 C 标签列表 */
    );
    return 1;   /* bit0=1 到这里 */

zero:              /* C 标签 */
    return 0;   /* bit0=0 跳到这里 */
}
```

## %l[label] 引用

```c
/* %l[label] 引用 C 标签的地址 */
asm goto(
    "cbz %w0, %l[empty]\n"    /* x=0 → 跳 empty */
    : : "r"(x) : : empty
);
```

| 语法 | 含义 |
|------|------|
| `%l[label]` | 引用 C 标签 `label` 的地址，汇编中跳到此处 |

## static_branch 机制

### 问题：条件分支的开销

```c
/* 普通条件判断 */
if (unlikely(debug_enabled)) {
    debug_print();
}
/* 编译为：
   ldr w0, [debug_enabled]
   cbz w0, skip          ← 分支预测可能错误
   bl debug_print
   skip:
*/
```

即使 `debug_enabled` 几乎总是 false，CPU 仍需要：
1. 加载 `debug_enabled` 变量
2. 分支预测（可能预测错误 → 流水线冲刷）

### static_branch 方案

```c
/* 内核 static_branch：默认 NOP，需要时改为 JMP */
DEFINE_STATIC_KEY_FALSE(debug_key);

if (static_branch_unlikely(&debug_key)) {
    debug_print();
}
```

**默认状态**（key=false）：
```asm
   nop                    ← 无开销！不需要加载变量、不需要分支
   bl debug_print         ← 这行被 NOP 跳过，不执行
```

**启用状态**（key=true）：
```asm
   b debug_print          ← NOP 被替换为 JMP
```

### static_branch 实现

```c
/* 简化的 static_branch 实现 */
static __always_inline bool static_branch_likely(struct static_key *key)
{
    asm_volatile_goto(
        "1: " ALTERNATIVE "nop\n"
        "   b %l[l_yes]\n"      /* 默认：NOP → 跳过，继续执行 */
        :                       /* 无输出 */
        : :                     /* 无输入/clobber */
        : l_yes                 /* 跳转目标 */
    );
    return false;  /* 默认路径：返回 false（不执行 if 体） */

l_yes:
    return true;   /* 启用路径：返回 true（执行 if 体） */
}
```

### 修改 static_branch

```c
/* 启用：把 NOP 改为 B */
static_branch_enable(&debug_key);
/* → patch NOP → B label */

/* 禁用：把 B 改为 NOP */
static_branch_disable(&debug_key);
/* → patch B → NOP */
```

修改后需要 **flush I-cache**（指令缓存一致性）：

```c
/* 修改代码后需要同步 I-cache */
flush_icache_range(addr, addr + 4);
```

## asm goto 的限制

| 限制 | 说明 |
|------|------|
| 无输出操作数 | 不能用 `"=r"` 返回值（通过跳转表达结果） |
| 跳转标签在 C 中 | 标签必须在同一个函数内 |
| 不能同时有输出和跳转 | GCC 5.12 之前完全不支持 |

> **GCC ≥ 5.12** 支持了 `asm goto` 和输出操作数同时使用（`asm goto with outputs`），内核 5.x 开始使用。

## 使用场景

| 场景 | 说明 |
|------|------|
| static_branch | 零开销条件分支（NOP vs JMP） |
| 空指针检查 | `tbz` 检查后跳到异常处理 |
| 快速路径 | 常见情况不跳转直接返回，罕见情况跳到 slow path |
| 替代 setjmp | 极少用，但理论上可以 |

## HFT 关联

- **static_branch 让调试代码默认零开销** → NOP 指令几乎不占时钟周期
- **比 if 判断更高效** → 无分支预测、无变量加载、无 cache miss
- **运行时动态切换** → 需要时 patch NOP→JMP 开启，不需要时 patch 回 NOP
- **tracing/profiling** → 性能监控代码默认零开销，需要时启用
- **I-cache 一致性** → patch 指令后必须 flush I-cache，否则可能执行旧指令

## 自测题

1. `asm goto` 和普通 `asm` 的区别？
<details><summary>答案</summary>
`asm goto` 可从汇编跳转到 C 标签（用 `%l[label]` 引用）。没有输出操作数——结果通过是否跳转来表达。普通 `asm` 只能在汇编内部跳转（用数字局部标号 1b/2f），不能跳到 C 代码。asm goto 用于 static_branch 等需要和 C 控制流联动的场景。
</details>

2. `static_branch` 如何实现零开销？
<details><summary>答案</summary>
默认放 NOP 指令在代码路径中——NOP 几乎不占时钟周期，CPU 直接跳过。需要启用时，内核把 NOP patch 为 `B label` 跳转指令。修改后 flush I-cache 保证一致性。所以默认状态（未启用）时几乎零开销。
</details>

3. `asm goto` 为什么（在 GCC 5.12 之前）不能有输出操作数？
<details><summary>答案</summary>
同时有输出操作数和跳转的复杂度太高：编译器需要管理两条执行路径（跳了 vs 没跳），每条路径上输出操作数是否有效？GCC 5.12 之前完全不支持。之后才支持 `asm goto with outputs`，但语义复杂：输出只在非跳转路径有效。
</details>

4. `%l[zero]` 中的 `l` 是什么含义？
<details><summary>答案</summary>
`l` 代表 label（标签）。`%l[zero]` 引用 C 标签 `zero` 的地址，汇编中用这个地址做跳转目标。这是 asm goto 的特有语法，普通 `asm` 不支持 `%l[]`。
</details>

5. 为什么 static_branch patch 指令后要 flush I-cache？
<details><summary>答案</summary>
CPU 的 I-cache 缓存了旧指令（NOP）。如果 patch 后不 flush I-cache，CPU 可能从 cache 中读到旧 NOP 而非新的 B 指令，导致行为不正确。`flush_icache_range` 让 I-cache 失效，强制从内存重新取指。
</details>

## 参考与延伸

- 原书 §10.5
- [10.3 常用实战示例](03-examples.md)
- 内核文档：Documentation/static-keys.txt
- GCC 文档：https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#Goto-Labels
