# 5.3 UBSAN：未定义行为检测

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

UBSAN (Undefined Behavior SANitizer) 检测 C 语言的**未定义行为** (UB)，如整数溢出、移位越界、除零等。

## 启用 UBSAN

```bash
# 内核配置
CONFIG_UBSAN=y
# 可选子选项:
CONFIG_UBSAN_BOUNDS=y          # 数组越界
CONFIG_UBSAN_SHIFT=y           # 移位溢出
CONFIG_UBSAN_DIV_ZERO=y        # 除零
CONFIG_UBSAN_SIGNED_OVERFLOW=y # 有符号整数溢出
CONFIG_UBSAN_BOOL=y            # 布尔值非法
CONFIG_UBSAN_ENUM=y            # 枚举值非法
CONFIG_UBSAN_ALIGNMENT=y       # 对齐违规
```

## UBSAN 检测的常见问题

```c
// 1. 有符号整数溢出 (UBSAN_SIGNED_OVERFLOW)
int a = INT_MAX;
int b = a + 1;  // UBSAN: signed integer overflow: 2147483647 + 1
// HFT 场景: 价格计算溢出 → 价格变负数 → 错误订单

// 2. 数组越界 (UBSAN_BOUNDS)
int arr[10];
arr[15] = 0;  // UBSAN: array index out of bounds

// 3. 移位溢出 (UBSAN_SHIFT)
int x = 1 << 32;  // UBSAN: shift exponent 32 is too large for 32-bit type
int y = 1 << 31;  // UBSAN: shift of 1 by 31 places cannot be represented in int

// 4. 除零 (UBSAN_DIV_ZERO)
int z = 100 / 0;  // UBSAN: division by zero

// 5. 布尔值非法
bool b = 42;  // UBSAN: load of value 42 is not a valid boolean

// 6. 对齐违规 (UBSAN_ALIGNMENT)
int *p = (int *)((char *)malloc(1) + 1);
*p = 42;  // UBSAN: misaligned access

// 7. 枚举值非法
enum color { RED, GREEN, BLUE };
enum color c = 42;  // UBSAN: load of value 42 is not a valid enum color
```

## UBSAN 报告示例

```
[   12.345678] ==================================================================
[   12.345680] UBSAN: signed-integer-overflow in calculate_checksum+0x48/0x80
[   12.345682] overflow of 2147483647 + 1 cannot be represented in type 'int'
[   12.345690] CPU: 2 PID: 1234 Comm: my_app
[   12.345695] Call trace:
[   12.345700]  calculate_checksum+0x48/0x80
[   12.345705]  my_handler+0x3c/0x100
[   12.345710]  __handle_irq_event_percpu+0x58/0x2a0
```

## 有符号溢出为什么是 UB

```c
// C 标准规定有符号整数溢出是未定义行为
// 编译器据此优化:

// 危险的"优化":
int check_overflow(int a, int b) {
    if (a + b < a)  // 编译器假设不溢出 → 优化为 false
        return -1;  // 这个分支被删除！
    return a + b;
}

// 正确的做法:
int check_overflow_safe(int a, int b) {
    if (b > 0 && a > INT_MAX - b)
        return -1;
    return a + b;
}

// 或使用无符号运算检查:
int check_overflow_unsigned(int a, int b) {
    int sum = (int)((unsigned)a + (unsigned)b);
    if ((b > 0 && sum < a) || (b < 0 && sum > a))
        return -1;
    return sum;
}
```

## UBSAN vs KASAN

| 维度 | KASAN | UBSAN |
|------|-------|-------|
| 检测类型 | 内存安全（越界/UAF） | 语言语义（溢出/除零） |
| 机制 | 影子内存 | 编译器插桩 |
| 开销 | 2-3x | 2-5% |
| 内存开销 | 1/8 | 极少 |
| 互补 | ✅ 可同时启用 | ✅ |

## HFT 关联

HFT 内核模块处理价格/数量数据时，整数溢出可能导致灾难性错误：

```c
// HFT 价格计算中的溢出风险
struct order {
    int32_t price;      // 价格（以 0.0001 为单位）
    int32_t quantity;   // 数量
    int64_t notional;   // 名义价值 = price * quantity
};

// 危险: price * quantity 可能溢出 int32
notional = (int64_t)price * quantity;  // 正确: 先转 int64

// UBSAN 可以检测到:
// int32_t temp = price * quantity;  // 溢出！
```

```bash
# HFT 开发环境推荐配置
CONFIG_UBSAN=y
CONFIG_UBSAN_SIGNED_OVERFLOW=y  # 检测价格计算溢出
CONFIG_UBSAN_SHIFT=y            # 检测位移越界
CONFIG_UBSAN_DIV_ZERO=y         # 检测除零
CONFIG_UBSAN_BOUNDS=y           # 检测数组越界
# CONFIG_UBSAN_ALIGNMENT=y      # 对齐检查（可能误报，按需）
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 有符号整数溢出为什么是未定义行为？编译器如何处理？

> C 标准规定有符号整数溢出是 UB，允许编译器假设不会溢出并据此优化。例如 `if (a + 100 < a)` 编译器可能直接优化为 `false`（假设不溢出）。这导致安全检查被优化掉。UBSAN 在运行时检测实际溢出并报告。

**Q2:** UBSAN 和 KASAN 有什么区别？

> KASAN 检测**内存安全**问题（越界、UAF、无效访问），通过影子内存实现。UBSAN 检测**语言语义**问题（整数溢出、移位越界、除零等），通过编译器插桩实现。两者互补：KASAN 管"访问的地址对不对"，UBSAN 管"计算的值对不对"。可以同时启用。

**Q3:** UBSAN 检测哪些类型的未定义行为？

> 整数溢出（signed）、数组越界、空指针访问、对齐违规、位运算未定义（如 shift > width）、bool 值非 0/1、枚举值越界。内核 CONFIG_UBSAN=y 启用，可按子选项选择检测类型。HFT 代码中的定点运算需注意整数溢出——UBSAN 可以帮发现。

**Q4:** UBSAN 的开销为什么比 KASAN 小？

> UBSAN 只在特定操作（算术运算、移位、除法）处插桩，不需要影子内存，内存开销极少。KASAN 对每次内存访问都检查影子内存，开销大。UBSAN 开销约 2-5%，KASAN 约 2-3x。

**Q5:** 为什么 `1 << 31` 在 int 类型上是 UB？

> `1` 是 int 类型（32位有符号），`1 << 31` 的结果 `0x80000000` 超过了 int 的正数范围（INT_MAX = 0x7FFFFFFF），是符号溢出。正确做法：`(unsigned int)1 << 31` 或 `1U << 31`。UBSAN_SHIFT 可以检测这类问题。

</details>

## 交叉引用

- [05.6 ch05 KASAN](../../chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch05 内存错误类型](../../chapter-05-memory-debug-1/notes/01-memory-error-types.md)
