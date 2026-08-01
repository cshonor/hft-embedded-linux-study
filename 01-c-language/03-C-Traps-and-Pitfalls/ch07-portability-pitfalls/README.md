# 第 7 章 可移植性缺陷

**Portability Pitfalls** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

[ch06 预处理器](../ch06-preprocessor/) 之后，本章聚焦 **换 CPU/OS/编译器后行为突变**：整型宽度、`char` 符号性、字节序、对齐、指针宽度、右移、OS API、浮点、NULL、平台宏 —— 嵌入式、跨架构驱动、多端工具链高频区。

## 小节索引

| 节 | 主题 |
|----|------|
| [7.1](./7.1-整型宽度.md) | `int`/`long` 不固定 → `stdint.h` |
| [7.2](./7.2-char符号性.md) | `char` signed vs unsigned |
| [7.3](./7.3-大小端.md) | `htonl` / 勿强转指针 |
| [7.4](./7.4-结构体对齐.md) | padding、pack、offsetof |
| [7.5](./7.5-指针宽度.md) | `uintptr_t` |
| [7.6](./7.6-有符号右移.md) | 算术 vs 逻辑右移 |
| [7.7](./7.7-OS与库差异.md) | POSIX vs Win32 vs RTOS |
| [7.8](./7.8-浮点差异.md) | FPU、NaN、比较 |
| [7.9](./7.9-NULL与地址空间.md) | NULL 解引用与平台 |
| [7.10](./7.10-平台宏.md) | `__x86_64__` / `_WIN32` 封装 |

## 跨平台编码规范

1. 协议/寄存器：**`stdint.h` 定宽类型**
2. 二进制字节：**`unsigned char` / `uint8_t`**
3. 跨设备传输：**网络字节序**，禁止 `*(uint32_t*)buf`
4. 硬件布局：**手动对齐** + `static_assert(offsetof)`
5. 指针整数化：**`uintptr_t`**
6. 移位：**无符号** 或显式掩码
7. 多 OS：**标准 C** + 薄平台抽象层
8. 不依赖默认 char 符号、对齐、有符号 `>>`

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch06 预处理器](../ch06-preprocessor/) |
| **后置** | [ch08 建议与答案](../ch08-advice-and-answers/) |
| **交叉** | [Expert C ch07 内存布局](../04-Expert-C-Programming/ch07-the-shapes-that-memory-takes/) |

## Demo

```bash
cd demo && make all
./demo01_stdint/main
./demo02_char_sign/main
./demo03_endian/main
./demo04_padding/main
./demo05_uintptr/main
./demo06_shift/main
./demo07_platform/main
```

## 面试题

1. LP64 下 `long` 几字节？为何时间戳用 `int64_t`？
2. `char c=0x80; c<0` 为何跨平台不一致？
3. 小端机如何读网络大端 `uint32_t`？
4. `struct { char a; int b; }` 为何 sizeof 常是 8？
5. 为何不能用 `int` 存 64 位指针？
