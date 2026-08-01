# TLPI 第 03 章 — System Programming Concepts

> 对应目录：`chapter-03-system-programming-concepts/`
> 书内标题：**System Programming Concepts**（系统编程概念）

**优先级**：🔴 必读  
**前置**：[Ch2 Fundamental Concepts](../chapter-02-basic-concepts/notes.md)  
**后置**：[Ch4 Universal I/O](../chapter-04-file-io-universal/notes.md)（第一个实战 syscall 集）  
**贯穿全书：** `tlpi_hdr.h`、错误处理函数会出现在所有示例里

---

## 章节定位

| | |
|--|--|
| **目标** | 用户空间 ↔ 内核交互模型、错误处理、可移植性规范 |
| **不是什么** | **没有** 文件读写实战（那是 Ch4） |

### 与 Ch4 边界（防混淆）

| 章 | 主题 | 内容 |
|----|------|------|
| **Ch3** | System Programming Concepts | 理论：syscall 模型、`errno`、头文件、可移植性 |
| **Ch4** | Universal I/O Model | 实战：`open/read/write/lseek`、fd |

→ 对照：[LKD §5.1 libc≠syscall](../../04-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-05-system-calls/notes/section-5.1-与内核通信.md)

---

## 3.1 System Calls 系统调用

**定义：** 用户进程进入内核的 **受控入口**；请求内核做受保护操作（读写、建进程、IPC…）。

| 特性 | |
|------|--|
| 模式切换 | **用户态 → 内核态** |
| 编号 | 每个 syscall 唯一号；内核用 `sys_call_table` 索引 |
| 谁触发陷阱 | 应用通常 **不** 手写 `syscall`；调 **glibc 包装函数** |

### 标准流程（x86 概念）

1. 应用调 glibc 包装（如 `open()`）  
2. 包装把 **调用号 + 参数** 放入寄存器  
3. 陷阱指令（`int 0x80` / `syscall`）进内核  
4. 内核入口：保存现场 → 校验 → 执行服务例程  
5. 回用户态；返回值给包装  
6. 包装判断错误码，失败则设 **`errno`**

> **开销：** 远贵于普通函数调用（切换、保存、校验）。HFT / 热路径要 **少 syscall**。

---

## 3.2 Library Functions（glibc）

| 类型 | 例子 | 是否进内核 |
|------|------|------------|
| 薄封装 | `read` / `write` | 通常一次 syscall |
| 纯用户态 | `strcpy` / `atoi` | **否** |
| 多层 | `fopen` | 内部 `open` + 用户态缓冲等 |

```
应用代码 → glibc 库函数 →（可选）系统调用 → 内核
```

**库函数 ≠ 系统调用。** Linux 上 C 标准库实现多为 **glibc**；标准 API 利于 UNIX 间移植。

---

## 3.3 错误处理（本章重中之重）

### `errno`（`<errno.h>`）

| 规则 | |
|------|--|
| 成功 | **不会** 把 `errno` 清零 |
| 失败 | 才被赋有意义的值 |
| 现代 glibc | **线程局部**；多线程互不干扰 |

```c
/* ❌ 错误：成功路径也可能留下旧 errno */
open(...);
if (errno != 0) { ... }

/* ✅ 先看返回值，失败再读 errno */
fd = open(path, O_RDONLY);
if (fd == -1) {
    /* 此时读 errno 才有意义 */
}
```

### 打印 / 退出工具

| 函数 | 作用 |
|------|------|
| `perror(msg)` | 打印 `msg` + errno 文本 |
| `strerror(errnum)` | 错误码 → 字符串（经典版非线程安全；有 `_r` 变体） |
| TLPI `errExit()` | 打印并 **exit** |
| TLPI `errMsg()` | 只打印不退出 |
| TLPI `fatal()` | 程序逻辑错误（非 syscall） |

### `tlpi_hdr.h`

原书示例统一引入：头文件集合、错误函数、数值解析等。  
清单概念：`lib/tlpi_hdr.h`、`error_functions.c` 等（见 man7 源码包）。

---

## 3.4 可移植编程（SUSv3 / POSIX）

### 功能测试宏（Feature Test Macros）

须放在 **所有 `#include` 之前**，控制 glibc 暴露哪些 API：

| 宏 | 含义 |
|----|------|
| `_POSIX_C_SOURCE` | POSIX |
| `_XOPEN_SOURCE` | SUS |
| `_GNU_SOURCE` | Linux / glibc 扩展（TLPI 示例常用） |

### 标准 typedef（`<sys/types.h>` 等）

| 类型 | 用途 |
|------|------|
| `pid_t` | 进程 ID |
| `off_t` | 文件偏移（`lseek`） |
| `size_t` / `ssize_t` | 长度；`read`/`write` 返回常用 `ssize_t` |
| `uid_t` / `gid_t` | 用户 / 组 ID |

勿随意写死 `int`/`long` 当跨 32/64 位 ID/偏移。

---

## 3.5 参数传递（概念）

| | |
|--|--|
| 用户指针 | 内核必须校验合法性 |
| 拷贝 | `copy_from_user` / `copy_to_user`（内核侧） |

→ [LKD §5.4](../../04-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-05-system-calls/notes/section-5.4-实现与参数验证.md)

---

## 3.6 原书示例清单（man7 源码）

| Listing | 内容 |
|---------|------|
| 3-1 | `tlpi_hdr.h` |
| 3-2 / 3-3 | `error_functions.h` / `.c` |
| 3-5 / 3-6 | `get_num.c` 安全字符串→数字 |
| — | `syscall_speed.c` 测 syscall 耗时 |

---

## 易混淆考点

1. `errno` 现代多为 **TLS**；成功不清零。  
2. 库函数失败 **不一定** 设 `errno`（看 man NOTES）。  
3. 多数 syscall 失败返回 `-1`；**少数 API 合法返回值可为负** — 以 man 为准。  
4. C 里的 `open()` 是 **包装**，不是直接陷阱指令。  
5. 用户态 **不能** 直接访问内核地址；只能靠 syscall。

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | 严格返回值+`errno`；功能测试宏保证 API 可见 |
| HFT | 少 syscall；测延迟时区分包装成本与真陷入 |

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | syscall = 进内核正规入口；经 glibc 包装 |
| 2 | 库函数 ≠ syscall；有的纯用户态 |
| 3 | 先判返回值，再读 `errno` |
| 4 | 功能测试宏放在 `#include` 最前 |
| 5 | Ch3 理论 · Ch4 才是 `open/read/write` 实战 |

---

## 参考

- 《The Linux Programming Interface》第 03 章 — System Programming Concepts  
- [OUTLINE](../OUTLINE.md) · [Ch4](../chapter-04-file-io-universal/notes.md)
