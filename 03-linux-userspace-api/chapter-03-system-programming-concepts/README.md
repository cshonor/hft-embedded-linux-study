# TLPI 第 03 章 — System Programming Concepts

**优先级**：🔴 必读  
**前置**：[Ch2 Fundamental Concepts](../chapter-02-basic-concepts/notes.md)  
**后置**：[Ch4 Universal I/O](../chapter-04-file-io-universal/notes.md)（第一个实战 syscall 集）  

---

## 小节目录

- [3.1 System Calls 系统调用](./notes/3.1-system-calls.md)
- [3.2 Library Functions（glibc）](./notes/3.2-library-functions-glibc.md)
- [3.3 错误处理（本章重中之重）](./notes/3.3-error-handling.md)
- [3.4 可移植编程（SUSv3 / POSIX）](./notes/3.4-susv3.md)
- [3.5 参数传递（概念）](./notes/3.5-concepts-parameter.md)

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

→ 对照：[LKD §5.1 libc≠syscall](../../05-linux-kernel/chapter-05-system-calls/notes/section-5.1-与内核通信.md)

---


---

## 3.6 原书示例清单（man7 源码）


| Listing | 内容 |
|---------|------|
| 3-1 | `tlpi_hdr.h` |
| 3-2 / 3-3 | `error_functions.h` / `.c` |
| 3-5 / 3-6 | `get_num.c` 安全字符串→数字 |
| — | `syscall_speed.c` 测 syscall 耗时 |

---


---

## 易混淆考点


1. `errno` 现代多为 **TLS**；成功不清零。  
2. 库函数失败 **不一定** 设 `errno`（看 man NOTES）。  
3. 多数 syscall 失败返回 `-1`；**少数 API 合法返回值可为负** — 以 man 为准。  
4. C 里的 `open()` 是 **包装**，不是直接陷阱指令。  
5. 用户态 **不能** 直接访问内核地址；只能靠 syscall。

---


---

## 双线提示


| 路线 | |
|------|--|
| 嵌入式 | 严格返回值+`errno`；功能测试宏保证 API 可见 |
| HFT | 少 syscall；测延迟时区分包装成本与真陷入 |

---


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


---

## 参考


- 《The Linux Programming Interface》第 03 章 — System Programming Concepts  
- [OUTLINE](../OUTLINE.md) · [Ch4](../chapter-04-file-io-universal/notes.md)


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>

/* Ch3 系统编程概念 — 演示用户态函数 vs 系统调用。
 * getpwuid() 是 libc 用户态函数（内部可能调 open/read）；
 * getuid() 是系统调用包装。
 * 编译: gcc -o ch3_demo ch3_demo.c */

int main(void) {
    /* 系统调用: 获取当前用户 ID */
    uid_t uid = getuid();
    printf("uid = %u\n", uid);

    /* libc 用户态函数: 将 uid 转为用户名 */
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        printf("username = %s\n", pw->pw_name);
        printf("home = %s\n", pw->pw_dir);
        printf("shell = %s\n", pw->pw_shell);
    }
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
