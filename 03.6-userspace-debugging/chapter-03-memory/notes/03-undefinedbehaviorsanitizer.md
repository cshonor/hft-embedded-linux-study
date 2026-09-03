# 3.3 UndefinedBehaviorSanitizer（UBSan 未定义行为）

> 🔴 精读 · 抓「不是内存错误、但同样致命」的未定义行为（UB）

## 本节要点

valgrind 抓内存错误，ASan 抓内存地址合法性，但有一类 bug 两者都**无感**：**未定义行为（Undefined Behavior, UB）**——有符号整数溢出、移位越界、除零、空指针解引用、类型转换越界等。UB 的可怕之处在于：程序「看似正常」甚至「debug 版正常、release 版诡异」，因为编译器会基于「UB 不会发生」的假设做激进优化。UndefinedBehaviorSanitizer（UBSan，`-fsanitize=undefined`）专治这类，本节讲它抓什么、怎么配、为什么 UB 是 HFT 的隐形杀手。

## 先看 UB 的两张「丑脸」

用 3.1 的 `mem_bugs.c` 里的雷 5、雷 6：

```c
void bug_signed_overflow(void) {
    int x = 2147483647;  // INT_MAX
    int y = x + 1;       // 有符号溢出 → UB
    printf("y=%d\n", y);
}

void bug_shift(void) {
    int x = 1 << 40;     // 移位数 >= 位宽(32) → UB
    printf("x=%d\n", x);
}
```

普通 `-O0` 编译运行，这两个雷**都不报错**，还给了你「看似合理」的结果：

```bash
gcc -g -O0 -o mem_bugs mem_bugs.c && ./mem_bugs
# y=-2147483648   ← 溢出「回绕」成最小负数
# x=0             ← 移位「悄悄」给了 0
```

问题来了：`-O0` 下溢出回绕，`-O2` 下编译器可能因为「UB 不可能发生」而**假设 `x+1 > x` 恒成立**，做出完全不同的优化，导致 debug/release 行为不一致。这就是 UB 的阴险——**它在 `-O0` 下"装没事"，在 `-O2` 下"原形毕露"**。

UBSan 登场：

```bash
gcc -g -O1 -fsanitize=undefined -o mem_bugs_ubsan mem_bugs.c
./mem_bugs_ubsan
```

```text
mem_bugs.c:29:34: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
mem_bugs.c:35:15: runtime error: shift exponent 40 is too large for 32-bit type 'int'
```

报告直接给**文件:行:列** + 一句话说清 UB 是什么。比「猜为什么 y 变成负数」强太多。

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

## 关键配置：让 UB「报错即停」而非「报完继续」

UBSan 默认行为是：打印报告后**继续运行**（可恢复）。这在「多个 UB 连锁」时会把报告淹没，也不利于 gdb 抓现场。用 `-fno-sanitize-recover` 让 UB 直接终止：

```bash
# 方式 1：所有 UBSan 检查遇到 UB 就 abort
gcc -g -O1 -fsanitize=undefined -fno-sanitize-recover=all -o mem_bugs_ubsan mem_bugs.c

# 方式 2：只让「溢出」这一个检查 abort，其余照常恢复
gcc -g -O1 -fsanitize=undefined -fno-sanitize-recover=signed-integer-overflow -o mem_bugs_ubsan mem_bugs.c
```

abort 后配合 gdb：

```bash
gdb ./mem_bugs_ubsan
(gdb) run
# Program received signal SIGABRT ...   ← 停在第一个 UB
(gdb) bt        # 看完整调用栈，定位是哪个调用路径触发了溢出
```

## 与 ASan 组合：`-fsanitize=address,undefined`

内存错误 + UB 常同时存在，两者可以一起开：

```bash
gcc -g -O1 -fsanitize=address,undefined -o mem_bugs_both mem_bugs.c
./mem_bugs_both
# 依次报 heap-buffer-overflow（ASan）→ signed integer overflow（UBSan）→ ...
```

这是开发期最省事的「全家桶」配置。注意：**不能和 TSan 同时开**（Ch4 会讲，TSan 需要独占），但 ASan+UBSan 兼容。

## 为什么 UB 是「隐形杀手」：编译器优化

很多人以为 UB「最多是结果不对」，低估了它的破坏力。实际上 UB 给了编译器**免责声明**：编译器有权假设 UB 永不发生，并基于此做「合法但反直觉」的优化。

经典例子——**溢出检查被优化掉**：

```c
// 程序员想写的「安全加法」：检查溢出
int safe_add(int a, int b) {
    if (a + b < a)      // 靠溢出回绕来检测溢出（错误做法）
        return -1;      // 认为溢出了
    return a + b;
}
```

`a + b` 若溢出是 UB，编译器据此**假设 `a+b` 永不溢出 → 假设 `a+b >= a` 恒成立 → 把 `if` 优化成恒假 → 直接删掉检查**。在 `-O2` 下，你精心写的「溢出保护」被编译器当死代码删了：

```bash
gcc -O2 -S safe_add.c -o - | grep -A5 safe_add
# 你会在汇编里看到 if 分支整个消失，只剩 return a + b
```

**这就是 debug 版正常、release 版出错的根本机制**。所以：
1. 不要「利用」UB 来做逻辑（比如用溢出回绕检测溢出）——编译器会反噬你。
2. 要用 `__builtin_add_overflow(a,b,&r)` 这类编译器提供的**无 UB 溢出检测**内建函数。
3. 开 UBSan 在开发期把 UB 钉死。

## HFT 关联

1. **价格/数量的累计溢出是错单根源**：`total_qty += order.qty` 这种累加，一旦 qty 或累计值超出 `int`/`long` 范围就 UB。HFT 里价格常用定点整数（如 `price * 10000` 存 `long`），溢出一个 `long` 就是一笔天价错单。UBSan 在仿真阶段就能抓出「哪个计算路径会溢出」。
2. **移位是算掩码/位域的常见雷**：`1 << bits` 里 `bits` 若等于 32（或负）就是 UB，可能导致掩码全 0 或全 1，进而错误地清掉/置上标志位。UBSan 的 `shift` 检查直接点名。
3. **除零导致 NaN/Inf 传播**：撮合引擎里 `avg = total / count`，count 为 0 时整数除零直接 SIGFPE 崩，浮点除零产生 NaN 污染后续所有计算。UBSan 的 `integer-divide-by-zero` / `float-divide-by-zero` 分别在开发期拦截。
4. **「debug 正常、release 错」的经典排查路径**：一旦出现这种症状，第一反应就该怀疑 UB——上 UBSan 重编译跑一遍，比在 `-O2` 反汇编里猜优化行为快得多。

```bash
# HFT 场景：定点价格累加溢出检查（正确做法，无 UB）
#include <stdbool.h>
bool add_price(long *acc, long delta) {
    long r;
    if (__builtin_add_overflow(*acc, delta, &r))  // 编译器内建，不触发 UB
        return false;   // 溢出，拒绝
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

> 编译器被允许**假设 UB 永不发生**，并基于此做激进优化。debug（`-O0`）几乎不优化，UB 表现为「回绕」等看似无害的结果；release（`-O2`）下编译器利用「UB 不可能」的假设删掉它认为「不可能走到」的代码——比如把「靠溢出回绕检测溢出」的 `if` 优化成恒假直接删除。于是同一个 UB，debug 版「装没事」，release 版「原形毕露」甚至删除你的保护逻辑。这是优化语义差异导致的，不是编译器 bug。

**Q3:** `-fno-sanitize-recover=all` 的作用？不加会怎样？

> 默认 UBSan 报完 UB 后**继续运行**（可恢复），适合「一次收集所有 UB」。加上 `-fno-sanitize-recover=all` 让第一个 UB 就直接 abort，好处是①避免后续连锁 UB 淹没报告、②配合 gdb 在第一个 UB 现场停下来 `bt` 抓调用栈。取舍：收集全景用默认恢复模式，定位单点用 abort 模式。

**Q4:** 为什么不能用「溢出回绕」来检测溢出（`if (a+b < a)`）？

> 因为 `a+b` 若真的溢出，这本身就是 UB，编译器可以假设它不发生，进而认为 `a+b < a` 恒为假，把整个 `if` 优化掉。正确做法是用编译器内建 `__builtin_add_overflow(a, b, &r)`（或 `<stdckdint.h>` 的 `ckd_add`），它在内部用无 UB 的方式检测溢出并返回布尔结果，不受优化影响。

**Q5:** ASan 和 UBSan 能一起开吗？和 TSan 呢？

> ASan 和 UBSan **可以**一起开：`-fsanitize=address,undefined`，两者兼容，是开发期「内存 + UB」全家桶。但**不能和 TSan（ThreadSanitizer）同时开**——TSan 需要独占运行时（它和 ASan 都接管内存分配/访问，会冲突），所以查并发竞态时要单独用 TSan 编译一次（见 Ch4）。

</details>

## 交叉引用

- [3.1 valgrind memcheck](01-valgrind-memcheck.md)
- [3.2 AddressSanitizer](02-addresssanitizer.md)
- [Ch4 并发类](../../chapter-04-concurrency/README.md)
- [Ch3 内存类](../README.md)
