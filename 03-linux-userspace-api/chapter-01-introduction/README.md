# TLPI 第 01 章 — History and Standards

**优先级**：🟡 选读（理清脉络；不必死记年表）  

---

## 小节目录

- 01.1 UNIX 的两层定义
- 01.2 UNIX & C 极简时间线
- 01.3 Linux = 内核 + GNU 工具链（必分清）
- 01.4 标准化：POSIX / SUS / LSB
- 01.5 贯穿全书：POSIX vs Linux 扩展
- 01.6 术语清单（极简）

---

## 章节定位


历史通识章：**无代码**。建立标准概念共识；快速通读即可。  
贯穿全书要分清：**POSIX 标准接口（跨 UNIX）** vs **Linux 独有扩展 API**。  
**Syscall 对外长什么样：** [1.x · 接口是 C / libc≠内核](./1.x-syscall-interface-is-c.md)  
**别和 Rust 普通库混：** [1.x · libc ≠ crate](./1.x-libc-vs-rust-crate.md)（表层像预制库，本质是 syscall 桥）

→ 全书定位：[../README.md](../README.md) · 下一章：[../chapter-02-basic-concepts/](../chapter-02-basic-concepts/)

---


---

## 7. 避坑


1. 本章无实操 — 理解「标准化意义」即可，勿深挖年表。  
2. **macOS 基于 BSD/XNU，不是 Linux**；POSIX 有重合，专属 API / 实现不同。  
3. `epoll` 代码 **不能** 直接当可移植写法搬到 macOS。

---


---

## 8. 自检


1. **用了 `epoll` 的代码能否直接在 macOS 编译运行？**  
   **不能。** `epoll` 是 Linux 特有；macOS 需用 `kqueue` 等另写。

2. **`pthread` 是 POSIX 还是 Linux 独有？**  
   **POSIX 标准**（Linux/macOS/BSD 均有实现；细节与扩展可不同）。

---


---

## 9. 背诵卡


| # | 要点 |
|---|------|
| 1 | 商标 UNIX ≠ 习惯「类 UNIX」；Linux 属后者（无 SUS 认证） |
| 2 | 发行版 = Linux 内核 + GNU 用户态 |
| 3 | **主力 POSIX**；SUS 了解即可；**LSB 忘掉** |
| 4 | POSIX 可移植；`epoll`/`io_uring` 等 Linux only |
| 5 | 嵌入式用户态偏 POSIX（驱动无 POSIX）；HFT 热路径可专吃 Linux 扩展 |

---


---

## 10. 参考


- 《The Linux Programming Interface》第 01 章 — History and Standards  
- [OUTLINE](../OUTLINE.md) · [模块 README](../README.md)


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* Ch1 历史/标准章 — 用最小程序演示"POSIX C 接口"长什么样。
 * write() 是 POSIX 标准系统调用（经 libc 包装）；
 * printf() 是 C 标准库（ISO C），底层最终也调 write()。
 * 编译: gcc -o ch1_demo ch1_demo.c */

int main(void) {
    /* POSIX 系统调用：直接经 libc 陷入内核 */
    if (write(STDOUT_FILENO, "hello via write()\n", 18) < 0)
        perror("write");

    /* ISO C 标准库：用户态缓冲，flush 时才调 write() */
    printf("hello via printf()\n");
    fflush(stdout);

    /* 检查 POSIX 标准是否定义了某些宏 */
#ifdef _POSIX_VERSION
    printf("_POSIX_VERSION = %ldL\n", (long)_POSIX_VERSION);
#else
    printf("POSIX not defined\n");
#endif
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
