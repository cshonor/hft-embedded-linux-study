# TLPI 第 02 章 — Fundamental Concepts

**优先级**：🔴 必读  

---

## 小节目录

- [2.1 内核（Kernel）](./notes/2.1-kernel.md)
- 2.2 用户态 / 内核态 / 系统调用
- [2.3 进程（Process）](notes/2.13-process-groups.md)
- [2.4 文件与文件描述符（FD）](notes/2.4-directory-hierarchy.md)
- [2.5 文件系统与 inode](notes/2.5-file-io-model.md)
- [2.6 权限模型](notes/2.5-file-io-model.md)
- [2.7 IPC（进程间通信）](notes/2.10-ipc.md)
- [2.8 信号（Signal）](notes/2.8-memory-mappings.md)
- [2.9 时间（两类时钟）](notes/2.16-date-time.md)

---

## 章节定位


全书核心地基：定义 UNIX/Linux 编程模型的核心术语；后面各章反复复用。  
打通 CSAPP 的「进程、虚拟内存、用户态/内核态」。

---


---

## 双线提炼


### 嵌入式 Linux 应用

- fd、权限、进程继承、`/dev` 设备文件 = 应用与驱动交互基础  
- 用户态 **不能** 绕过 syscall 直接操作硬件  

### HFT 低延迟

1. 少 syscall、少线程切换（上下文切换成本）  
2. 测耗时：**别用墙上时钟**  
3. 信号要管控，避免异步打断热路径  
4. 理解 VA 隔离 → 后续 `mlock` 防换页  

---


---

## 自测（答案）


1. **为何进程不能直接访问对方内存？靠什么隔离？**  
   各有独立 **虚拟地址空间**；由 **内核 + MMU/页表** 隔离保护。

2. **系统调用为何有性能开销？**  
   特权级切换、保存/恢复上下文、进内核执行路径 — **上下文切换** 成本。

3. **stdin/stdout/stderr 编号？**  
   **0 / 1 / 2**。

4. **硬链接 vs 软链接？**  
   硬链接：多名字 → **同一 inode**；软链接：存 **路径字符串**（可指不存在目标）。

5. **测代码耗时用哪种时钟？**  
   用 **单调/CPU 相关计时**（如 `CLOCK_MONOTONIC` 一类）；**不要**用会跳变的墙上日历时间当唯一依据。书中「CPU 时间」适合看占核；测墙钟耗时用 **单调时钟** 更稳妥。

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 用户态受限；进内核靠 **syscall** |
| 2 | 进程 = 独立 VA + 资源集合；线程共享 VA |
| 3 | 一切皆文件；fd 0/1/2；fork 继承 fd |
| 4 | inode 无文件名；硬链同 inode，软链存路径 |
| 5 | HFT：少 syscall；测时忌墙上时钟跳变 |

---


---

## 参考


- 《The Linux Programming Interface》第 02 章 — Fundamental Concepts  
- [OUTLINE](../OUTLINE.md) · [模块 README](../README.md)  
- 下一章方向：系统调用与库函数（Ch3）


---

## 代码示例

```c
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* Ch2 基本概念 — 演示 errno 错误处理 + 用户态/内核态切换。
 * open() 失败时 errno 被设置；strerror 将 errno 转为可读字符串。
 * 编译: gcc -o ch2_demo ch2_demo.c */

int main(void) {
    /* 故意打开不存在的文件，触发 errno */
    int fd = open("/nonexistent/file", O_RDONLY);
    if (fd < 0) {
        printf("open failed: errno=%d (%s)\n", errno, strerror(errno));
        perror("perror also works");
    }

    /* 演示: 同一系统调用成功时 errno 不被清零 */
    fd = open("/etc/hostname", O_RDONLY);
    if (fd >= 0) {
        char buf[256];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("hostname: %s", buf);
        }
        close(fd);
    }
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
