# 5.3 UBSAN：未定义行为检测

> 🔴 精读

## 本节要点

### UBSAN (Undefined Behavior SANitizer)

UBSAN 检测 C 语言的**未定义行为** (UB)，如整数溢出、空指针、数组越界等。

### 启用 UBSAN

```bash
# 内核配置
CONFIG_UBSAN=y
# 可选子选项:
CONFIG_UBSAN_BOUNDS=y       # 数组越界
CONFIG_UBSAN_SHIFT=y        # 移位溢出
CONFIG_UBSAN_DIV_ZERO=y     # 除零
CONFIG_UBSAN_SIGNED_OVERFLOW=y  # 有符号整数溢出
CONFIG_UBSAN_BOOL=y         # 布尔值非法
CONFIG_UBSAN_ENUM=y         # 枚举值非法
```

### UBSAN 检测的常见问题

```c
// 1. 有符号整数溢出 (UBSAN 检测)
int a = INT_MAX;
int b = a + 1;  // UBSAN: signed integer overflow

// 2. 数组越界 (UBSAN_BOUNDS)
int arr[10];
arr[15] = 0;  // UBSAN: array index out of bounds

// 3. 移位溢出 (UBSAN_SHIFT)
int x = 1 << 32;  // UBSAN: shift exponent too large

// 4. 空指针 (部分检测)
int *p = NULL;
*p = 42;  // 内核会 Oops，UBSAN 可能也报告

// 5. 布尔值非法
bool b = 42;  // UBSAN: load of value 42 is not a valid boolean
```

### UBSAN 报告示例

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

### HFT 关联

HFT 内核模块处理价格/数量数据时，整数溢出可能导致灾难性错误（如价格变负数）。UBSAN 能在开发期捕获这些错误。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 有符号整数溢出为什么是未定义行为？编译器如何处理？

> C 标准规定有符号整数溢出是 UB，允许编译器假设不会溢出并据此优化。例如 `if (a + 100 < a)` 编译器可能直接优化为 `false`（假设不溢出）。这导致安全检查被优化掉。UBSAN 在运行时检测实际溢出并报告。

**Q2:** UBSAN 和 KASAN 有什么区别？

> KASAN 检测**内存安全**问题（越界、UAF、无效访问），通过影子内存实现。UBSAN 检测**语言语义**问题（整数溢出、移位越界、除零等），通过编译器插桩实现。两者互补：KASAN 管"访问的地址对不对"，UBSAN 管"计算的值对不对"。可以同时启用。

</details>
