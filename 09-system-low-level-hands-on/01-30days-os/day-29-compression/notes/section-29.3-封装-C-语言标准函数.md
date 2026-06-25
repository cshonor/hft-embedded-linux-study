## ③ 封装 C 语言标准函数

在 **INT 0x40 API** 之上再包一层 **C 标准库风格**：

| 函数 | 底层 |
|------|------|
| **`putchar` / `printf`** | EDX 1/2/3… 显示 API |
| **`exit`** | 结束 app / RETF 链 |
| **`malloc` / `free`** | EDX 9/10 |

**app 写「像 POSIX 一点」的 C** — 不直接 **`_api_*` + EDX`** — **可移植感↑**。

→ [Day 27 apilib.lib](../day-27-ldt-lib/) · [Day 20 API](../day-20-api/)

---
