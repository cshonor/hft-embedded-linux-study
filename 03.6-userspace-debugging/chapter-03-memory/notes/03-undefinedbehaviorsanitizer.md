# 3.3 UndefinedBehaviorSanitizer（UBSan 未定义行为）

> 🔴 精读 · 抓「不是内存错误、但同样致命」的未定义行为（UB）

## 本节要点

valgrind 抓内存错误，ASan 抓内存地址合法性，但有一类 bug 两者都**无感**：**未定义行为（Undefined Behavior, UB）**——有符号整数溢出、移位越界、除零、空指针解引用、类型转换越界等。UB 的可怕之处在于：程序「看似正常」——**不崩、不报错，只是给你一个看起来合理的错值**（实测 `1 << 40` 得到 `256`，不是 0）。更深一层，编译器会基于「UB 不会发生」的假设做变换，把「利用 UB 写的逻辑」**等价改写成完全不同的东西**（实测把「靠回绕检测溢出」的 `if (a+b<a)` 改成 `if (b<0)`，而且 `-O0` 就改写）。UndefinedBehaviorSanitizer（UBSan，`-fsanitize=undefined`）专治这类，本节讲它抓什么、怎么配、为什么 UB 是 HFT 的隐形杀手。**本节所有输出均为 gcc 13.3.0 实测。**

## 先看 UB 的四种「丑脸」（实测）

`code/c3_3_ubsan_ops.c` 把四类最值得先查的 UB 装进一个程序（都用 `volatile` 逼编译器**真去执行**这些运算，否则会被常量折叠掉）：

```c
/* code/c3_3_ubsan_ops.c 节选 */
static volatile int  v_int_max = 2147483647;                /* INT_MAX */
static volatile int  v_one = 1;
static volatile int  v_shift = 40;
static volatile int  v_zero = 0;
static volatile long v_min = -9223372036854775807L - 1;     /* LONG_MIN */

int a = v_int_max;
int overflow = a + v_one;                     /* ① 有符号溢出：UB */
printf("① INT_MAX + 1 = %d   （数学上是 2147483648，int 装不下）\n", overflow);
fflush(stdout);

int b = v_one << v_shift;                     /* ② 移位量 ≥ 位宽：UB */
printf("② 1 << 40 = %d          （int 只有 32 位）\n", b);
fflush(stdout);

long d = v_min;
long negate = -d;                             /* ③ 对 LONG_MIN 取负：UB */
printf("③ -LONG_MIN = %ld\n", negate);
fflush(stdout);

volatile int one = v_one;
volatile int zero = v_zero;
int c = one / zero;                           /* ④ 除零：UB（唯一当场送命的） */
printf("④ 1 / 0 = %d           （能打出来才怪）\n", c);
```

**不加 sanitizer**（`gcc -g -O0 -Wall -Wextra`）——**编译器一条警告都不给**，程序还给了你四个「看似合理」的结果，退出码 **136**：

```text
① INT_MAX + 1 = -2147483648   （数学上是 2147483648，int 装不下）
② 1 << 40 = 256          （int 只有 32 位）
③ -LONG_MIN = -9223372036854775808
对照：1L << 40 = 1099511627776（合法，没有 UB）
--- stderr ---
Program terminated with signal SIGFPE (8)
```

**四条里三条「装没事」，而且第二条的错法尤其阴**：

| UB | 你以为会是 | 实测输出 | 真实原因 |
|----|-----------|----------|----------|
| ① `INT_MAX + 1` | 崩溃 | `-2147483648` | 补码回绕 |
| ② `1 << 40` | `0`（「移出去了」） | **`256`** | x86 的 `shl` 对 32 位操作数**只用移位数低 5 位**：`40 & 31 = 8`，等于 `1 << 8` |
| ③ `-LONG_MIN` | `9223372036854775808` | `-9223372036854775808` | 取负后回绕成自己 |
| ④ `1 / 0` | 崩溃 | （无输出） | 只有它真发 SIGFPE |

> 📌 **② 那条值得单独记住**：它给出的不是「一看就不对」的 0，而是 `256`——一个**看起来完全正常的数**。如果这个移位是算掩码（`mask = 1 << bits`），你会拿到一个「合法但错」的掩码，然后**错误地置上/清掉标志位**。UB 最危险的形态就是「还给你一个合理的数」。

**加上 UBSan**（`gcc -g -O1 -fsanitize=undefined`）——四条 UB 被逐条点名，退出码仍是 **136**：

```text
--- stdout ---
① INT_MAX + 1 = -2147483648   （数学上是 2147483648，int 装不下）
② 1 << 40 = 256          （int 只有 32 位）
③ -LONG_MIN = -9223372036854775808
对照：1L << 40 = 1099511627776（合法，没有 UB）
--- stderr ---
/app/example.c:31:9: runtime error: signed integer overflow: 1 + 2147483647 cannot be represented in type 'int'
/app/example.c:35:19: runtime error: shift exponent 40 is too large for 32-bit type 'int'
/app/example.c:40:10: runtime error: negation of -9223372036854775808 cannot be represented in type 'long int'; cast to an unsigned type to negate this value to itself
/app/example.c:54:17: runtime error: division by zero
Program terminated with signal SIGFPE (8)
```

报告直接给**文件:行:列** + 一句话说清 UB 是什么。比「猜为什么 `1<<40` 变成 256」强太多。

> ⚠️ **为什么 stdout 里那四行还能看见？** 因为源码里每条 `printf` 后面都跟了 `fflush(stdout)`。这不是啰嗦——**④ 除零会发 SIGFPE 直接杀进程，而 stdout 接管道时是全缓冲的**：不刷的话前面四条「错值」全留在缓冲区里随进程一起消失，你只能看到一份 UBSan 报告、看不到任何程序输出。`c1_2_shrink_demo.c` 里那句 `fflush(stdout)` 是同一个道理。
>
> 同一个坑还解释了下面 `-fno-sanitize-recover=all` 那次运行：**stdout 是空的**——因为第一个 UB（第 31 行的溢出）就在第一次 `printf` **之前**，UBSan 当场 abort，什么都还没来得及刷。

## UBSan 抓什么：UB 清单

`-fsanitize=undefined` 是一组检查的合集，GCC/Clang 支持以下子项（可单独开关）：

| 检查项 | 捕获的 UB | 典型场景 |
|--------|-----------|----------|
| `signed-integer-overflow` | 有符号整数溢出 | 价格/数量累加溢出（最常见） |
| `shift` | 移位越界（移位数 <0 或 ≥位宽） | `1 << n` 里 n 算错 |
| `integer-divide-by-zero` | 整数除零 | 除数变量为 0（浮点除零不算 UB） |
| `bounds` | 数组越界（编译期可知的） | `arr[i]` 静态数组越界 |
| `null` | 空指针解引用 | `*p` 且 p=NULL（Clang 支持更好） |
| `float-cast-overflow` | 浮点↔整数转换越界 | `(int)1e20` 溢出 |
| `float-divide-by-zero` | 浮点除零 | `1.0/0.0` |
| `alignment` | 未对齐访问 | 结构体指针强转后解引用 |
| `enum` | 枚举值越界 | 给 enum 赋了范围外值 |
| `bool` | bool 变量赋了 0/1 之外的值 | 用 `*((bool*)&x)` 之类 |
| `vptr` | 多态类型错误 | 基类指针指向未构造对象（C++） |
| `nonnull-attribute` | 违反 `__attribute__((nonnull))` | 传 NULL 给声明 nonnull 的函数 |
| `returns-nonnull-attribute` | 违反 returns-nonnull | 函数承诺返回非空却返 NULL |
| `unreachable` | 执行到 `__builtin_unreachable()` | 逻辑走到不该走的分支 |

> 完整清单看编译器手册 `man gcc` 搜 `-fsanitize`。核心记忆：**有符号溢出 + 移位 + 除零** 是最值得先查的三项，它们覆盖了 HFT 里 90% 的 UB 事故。

## ⚠️ 反直觉实测：`integer-divide-by-zero` 不一定「发得出来」

上表里 `integer-divide-by-zero` 看着最「实」——除零不就是硬件异常吗？实测告诉你**不一定**。同一份 `probe`，两种写法在 `-O0` 下行为完全不同：

| 写法 | 生成指令 | 实测 |
|------|---------|------|
| `g_one / g_zero`（两个运行时值） | `cdq` + `idiv ecx` | **SIGFPE，退出 136** |
| `1 / g_zero`（**分子是编译期常量**） | 一段**不含任何除法**的比较/条件搬移序列 | **不崩，打出 0，退出 0** |

`idiv` 才是发 `#DE`（除零异常）的那条指令；常量分子被折掉后程序里**根本没有除法**，自然也就没有除零异常——它「算出」的 0 只是那段无分支序列对 UB 输入的任意产物。

**这条对 UBSan 使用者有个实际影响**：既然「除零」在编译期就可能被折叠掉，那 `-fsanitize=integer-divide-by-zero` 也就**未必能报出来**（UBSan 是运行时检查，没有除法指令就没有检查点）。本节的 `c3_3_ubsan_ops.c` 之所以能稳定报出 `division by zero`（第 54 行），正是因为它**特意用了两个 volatile**（`one` / `zero`）逼编译器保留 `idiv`。

```c
/* 想稳定复现除零 UB，两个操作数都必须是运行时值 */
volatile int one  = v_one;
volatile int zero = v_zero;
int c = one / zero;        /* ← 这样才有 idiv，才发得出 SIGFPE */
```

> 完整推演（含两种写法的汇编对照、以及「假设被证伪」的调试循环）见 [1.4 调试元流程](../../chapter-01-methodology/notes/04-debugging-process.md) 的「动手」一节，源码 `chapter-01-methodology/code/c1_4_hypothesis_div.c`。

## 关键配置：让 UB「报错即停」而非「报完继续」

UBSan 默认行为是：打印报告后**继续运行**（可恢复）。这在「多个 UB 连锁」时会把报告淹没，也不利于 gdb 抓现场。用 `-fno-sanitize-recover` 让 UB 直接终止：

```bash
# 方式 1：所有 UBSan 检查遇到 UB 就 abort
gcc -g -O1 -fsanitize=undefined -fno-sanitize-recover=all -o c3_3_ubsan_stop c3_3_ubsan_ops.c

# 方式 2：只让「溢出」这一个检查 abort，其余照常恢复
gcc -g -O1 -fsanitize=undefined -fno-sanitize-recover=signed-integer-overflow \
    -o c3_3_ubsan_ovf c3_3_ubsan_ops.c
```

实测方式 1（退出码 **1**，注意 **stdout 是空的**）：

```text
--- stdout ---
（空）
--- stderr ---
/app/example.c:31:9: runtime error: signed integer overflow: 1 + 2147483647 cannot be represented in type 'int'
```

对比默认（recover）模式报出 4 条后仍跑到 SIGFPE，这里**只报第一条就 abort 了**。而 stdout 为空正是上面说过的缓冲问题：第一个 UB（第 31 行的溢出）出现在第一次 `printf` **之前**，进程在刷缓冲前就死了。

abort 后配合 gdb：

```bash
gdb ./c3_3_ubsan_stop
(gdb) run
# Program received signal SIGABRT ...   ← 停在第一个 UB
(gdb) bt        # 看完整调用栈，定位是哪个调用路径触发了溢出
```

## 与 ASan 组合：`-fsanitize=address,undefined`

内存错误 + UB 常同时存在，两者可以一起开：

```bash
gcc -g -O1 -fsanitize=address,undefined -o prog_both prog.c
./prog_both
# ASan 报内存问题（exit 1），UBSan 报运算 UB（默认继续跑，遇到 SIGFPE 才死）
```

这是开发期最省事的「全家桶」配置。注意：**不能和 TSan 同时开**（Ch4 会讲，TSan 需要独占），但 ASan+UBSan 兼容。

> 📌 **退出码要分开理解**：ASan 检出问题 → **exit 1**（它主动 `_exit(1)`）；UBSan 默认只是**打印**（不影响退出码），除非 UB 本身引发了硬件异常（除零 → SIGFPE → **136**）或你开了 `-fno-sanitize-recover`（→ exit 1 / SIGABRT）。三种退出码混在一起时，先看 stderr 是「谁」在报。

## 为什么 UB 是「隐形杀手」：编译器优化（实测）

很多人以为 UB「最多是结果不对」，低估了它的破坏力。实际上 UB 给了编译器**免责声明**：编译器有权假设 UB 永不发生，并基于此做「合法但反直觉」的变换。

经典例子——**「靠溢出回绕检测溢出」的写法**：

```c
int safe_add_wrap(int a, int b) {
    if (a + b < a)      /* 靠溢出回绕来检测溢出（错误做法） */
        return -1;
    return a + b;
}
```

`a + b` 若溢出是 UB，编译器据此**假设 `a+b` 永不溢出**。于是 `a+b < a` ⟺ `b < 0`——**整个检查被等价改写成「b 是不是负数」**。实测 gcc 13.3 的 `-O2` 汇编（`code` 见 `probe36/p_safeadd.c`）：

```asm
safe_add_wrap:
        test    esi, esi          ; 测 b 的符号
        js      .L3               ; b < 0 → 返回 -1
        lea     eax, [rsi+rdi]    ; 否则返回 a + b（溢出了也不管）
        ret
.L3:
        mov     eax, -1
        ret
```

**注意措辞：检查没有「消失」，它变成了另一个东西。** 很多教材说「`if` 分支被删掉」（原文本节也这么写），但实测 gcc 13.3 是**等价改写**——分支还在，语义已经不是溢出检测了。更狠的是：**这个改写连 `-O0` 都做**（`cmp DWORD PTR [rbp-8], 0` / `jns`，同样是「b < 0」）。它不是优化级别的行为，是**前端就完成的 UB 折叠**。

后果是**双向错误**。实测运行结果（`-O0` 和 `-O2` 完全一致）：

```text
safe_add_wrap(INT_MAX, 1)   = -2147483648   （真溢出了，应该报 -1）
safe_add_wrap(5, -3)        = -1   （根本没溢出，不该拦）
----
safe_add_builtin(INT_MAX, 1)= -1   （内建版：正确拦住）
safe_add_builtin(5, -3)     = 2   （内建版：正常放行）
```

| 输入 | 这个「保护」的判定 | 应该的判定 | 结论 |
|------|-------------------|-----------|------|
| `(INT_MAX, 1)` | 放行，返回回绕值 `-2147483648` | 拦住（真溢出） | **漏报**：天价错单的来源 |
| `(5, -3)` | 拦住，返回 `-1` | 放行（没溢出） | **误报**：正常交易被拒 |

**保护逻辑变成了「b 是不是负数」——它和溢出已经完全无关。**

正确做法是编译器内建 `__builtin_add_overflow`（`-O2` 下用 CPU 的溢出标志 `cmovno`，一次 `add` + 一条条件搬移搞定）：

```asm
safe_add_builtin:
        add     edi, esi          ; 真正的加法，CPU 同时置 OF 标志
        mov     eax, -1
        cmovno  eax, edi          ; 没溢出（OF=0）才取和，否则保留 -1
        ret
```

所以三条结论：

1. **不要「利用」UB 做逻辑**（用溢出回绕检测溢出、用移位当掩码边界）——编译器会反噬你，而且**在 `-O0` 就反噬**。
2. **要用 `__builtin_add_overflow(a,b,&r)`**（或 C23 的 `<stdckdint.h>` / `ckd_add`），它内部用无 UB 的方式检测并返回布尔结果，不受优化影响。
3. **开 UBSan 在开发期把 UB 钉死**——因为这类改写「不报错、不崩溃、结果还像个正常数」，你不开工具根本发现不了。

## HFT 关联

1. **价格/数量的累计溢出是错单根源**：`total_qty += order.qty` 这种累加，一旦 qty 或累计值超出 `int`/`long` 范围就 UB。HFT 里价格常用定点整数（如 `price * 10000` 存 `long`），溢出一个 `long` 就是一笔天价错单。UBSan 在仿真阶段就能抓出「哪个计算路径会溢出」。
2. **移位是算掩码/位域的常见雷**：`1 << bits` 里 `bits` 若 ≥ 32（或负）就是 UB。实测 gcc 13.3 下 `1 << 40` 给出的是 **`256`**（x86 `shl` 只用移位数低 5 位：`40 & 31 = 8`）——**不是一个一眼可疑的 0，而是一个看似正常的掩码**，于是你会错误地置上/清掉标志位。UBSan 的 `shift` 检查直接点名 `shift exponent 40 is too large for 32-bit type 'int'`。
3. **除零导致 NaN/Inf 传播**：撮合引擎里 `avg = total / count`，count 为 0 时整数除零发 SIGFPE 崩（实测退出码 136），浮点除零产生 NaN 污染后续所有计算。UBSan 的 `integer-divide-by-zero` / `float-divide-by-zero` 分别在开发期拦截。**但别以为「除零必崩」**——分子是编译期常量时编译器会折成不含 `idiv` 的序列，连崩都不崩（见自测题 Q6）。
4. **「debug 正常、release 错」的经典排查路径**：一旦出现这种症状，第一反应就该怀疑 UB——上 UBSan 重编译跑一遍，比在 `-O2` 反汇编里猜优化行为快得多。

```c
/* HFT 场景：定点价格累加溢出检查（正确做法，无 UB） */
#include <stdbool.h>

bool add_price(long *acc, long delta) {
    long r;
    if (__builtin_add_overflow(*acc, delta, &r))  /* 编译器内建，不触发 UB */
        return false;   /* 溢出，拒绝 */
    *acc = r;
    return true;
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 valgrind 和 ASan 都抓不到「有符号整数溢出」？

> 因为它们关注的是**内存**：valgrind 查「地址可不可访问 + 值有没有初始化」，ASan 查「地址是否在红区/已 free」。有符号溢出不涉及任何非法内存访问——`x+1` 只是算出一个「错」的值，地址完全合法、值也初始化了，所以两者无感。溢出属于**未定义行为**而非**内存错误**，是 UBSan 的专属领域。三者维度不同：ASan 管地址、MSan 管值、UBSan 管「运算语义」。

**Q2:** UB 的「隐形」体现在哪？为什么 debug 版正常、release 版诡异？

> 编译器被允许**假设 UB 永不发生**，并基于此做「合法但反直觉」的变换。UB 的典型表现不是崩溃，而是**回绕出看似合理的值**（`INT_MAX+1 = -2147483648`、`1<<40 = 256`）；更麻烦的是编译器会**把「利用 UB 写的逻辑」等价改写成别的东西**——比如把「靠回绕检测溢出」的 `if (a+b < a)` 改写成 `if (b < 0)`，检查还在但语义完全变了。实测这条改写**在 `-O0` 就发生**，所以不只是「debug 正常、release 诡异」，而是「debug 和 release 都错，只是错得不一样」：`safe_add_wrap(INT_MAX,1)` 漏报（返回回绕值）、`safe_add_wrap(5,-3)` 误报（返回 -1）。这是 UB 语义导致的，不是编译器 bug。

**Q3:** `-fno-sanitize-recover=all` 的作用？不加会怎样？

> 默认 UBSan 报完 UB 后**继续运行**（可恢复），适合「一次收集所有 UB」——实测默认模式下 4 条 UB 全部报出。加上 `-fno-sanitize-recover=all` 让第一个 UB 就直接 abort（实测：只报第一条、退出码 1、**stdout 为空**，因为第一个 UB 在第一次 `printf` 之前就终止了进程）。好处是①避免后续连锁 UB 淹没报告、②配合 gdb 在第一个 UB 现场停下来 `bt` 抓调用栈。取舍：收集全景用默认恢复模式，定位单点用 abort 模式。

**Q4:** 为什么不能用「溢出回绕」来检测溢出（`if (a+b < a)`）？

> 因为 `a+b` 若真的溢出，这本身就是 UB，编译器可以假设它不发生，从而把 `a+b < a` 等价改写成 `b < 0`。于是这个「保护」**双向失效**：真溢出时（`b >= 0`）漏报并返回回绕值；没溢出时（`b < 0`）反而误报返回 -1——实测 `safe_add_wrap(INT_MAX,1) = -2147483648`、`safe_add_wrap(5,-3) = -1`。正确做法是用编译器内建 `__builtin_add_overflow(a, b, &r)`（或 `<stdckdint.h>` 的 `ckd_add`），它在内部用无 UB 的方式检测溢出并返回布尔结果，不受优化影响（实测两种输入都判定正确）。

**Q5:** 本节实测里 `1 << 40` 给出的是 `256` 而不是 `0`，为什么？这件事为什么比「给出 0」更危险？

> 因为 x86 的 `shl` 指令对 32 位操作数**只取移位数的低 5 位**：`40 & 31 = 8`，于是执行的是 `1 << 8 = 256`。比「给出 0」更危险的地方在于：`0` 是一眼就不对的值，而 `256` **看起来是个完全正常的数**。如果这行是算掩码（`mask = 1 << bits`），你会拿到一个「合法但错」的掩码，然后错误地置上/清掉标志位——错误会一路传播下去，且不触发任何崩溃或告警。UB 最危险的形态就是「还给你一个合理的数」。

**Q6:** 整数除零一定发 SIGFPE 吗？

> **不一定。** 这取决于编译器有没有为这次除法生成 `idiv` 指令。实测（gcc 13.3 `-O0`）：`g_one / g_zero`（两个运行时值）生成 `cdq` + `idiv` → 除零 → `#DE` → SIGFPE；但 `1 / g_zero`（**分子是编译期常量**）被折成一段**不含任何除法指令**的无分支序列，结果「不崩、直接打出 0」，退出码 0。根因是编译器有权基于「除数非零」这个 UB 假设做变换。完整推演（含两种写法的汇编对照）见 [1.4 调试元流程](../../chapter-01-methodology/notes/04-debugging-process.md) 的「动手」一节。

**Q5:** ASan 和 UBSan 能一起开吗？和 TSan 呢？

> ASan 和 UBSan **可以**一起开：`-fsanitize=address,undefined`，两者兼容，是开发期「内存 + UB」全家桶。但**不能和 TSan（ThreadSanitizer）同时开**——TSan 需要独占运行时（它和 ASan 都接管内存分配/访问，会冲突），所以查并发竞态时要单独用 TSan 编译一次（见 Ch4）。

</details>

## 交叉引用

- [3.1 valgrind memcheck](01-valgrind-memcheck.md)
- [3.2 AddressSanitizer](02-addresssanitizer.md)
- [Ch4 并发类](../../chapter-04-concurrency/README.md)
- [Ch3 内存类](../README.md)
