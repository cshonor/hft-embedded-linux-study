# TLPI 第 25 章 — Process Termination

**优先级**：🔴（fork 后退出选型、退出码、僵尸衔接）  
**前置**：[Ch24 fork](../chapter-24-process-creation/notes.md)  
**后置**：[Ch26 wait / 僵尸](../chapter-26-monitoring-child-processes/notes.md) · [Ch27 exec](../chapter-27-program-execution/notes.md)

---

## 小节目录

- [25.1 终止分类](./notes/25.1-termination-classification.md)
- [25.2 `exit` vs `_exit` / `_Exit`（核心）](./notes/25.2-exit-exit-exit.md)
- [25.3 `main` return](./notes/25.3-main.md)
- [25.4 退出状态](./notes/25.4-state.md)
- [25.5 `atexit` / `on_exit`](./notes/25.5-atexit-onexit.md)
- [25.6 内核销毁时做什么](./notes/25.6-section-25-6.md)
- [25.7 僵尸（衔 Ch26）](./notes/25.7-ch26.md)
- [25.8 `abort`](./notes/25.8-abort.md)

---

## 章节目标


区分正常/异常终止；对比 `exit` / `_exit` / `_Exit`；掌握退出状态与 atexit；理解内核回收与僵尸；衔接 wait。

---


---

## 25.9 易错清单


1. fork 子用 `exit` → atexit 双跑  
2. `_exit` 不刷 stdio → 输出可能丢  
3. 退出码截断到 8 位  
4. 信号杀跳过全部用户清理；`SIGKILL` 尤甚  
5. `on_exit` 不可移植；优先 `atexit`  

---


---

## 速查


| API | 回调 | 刷 stdio | 典型用途 |
|-----|------|----------|----------|
| `exit` | ✅ | ✅ | 正常结束进程 |
| `_exit`/`_Exit` | ❌ | ❌ | fork 子、信号敏感路径 |
| `abort` | 经信号 | — | 异常自毁 |

---


---

## 练习


1. atexit 逆序  
2. `exit` vs `_exit` 缓冲差异  
3. fork + `exit` 复现双 atexit；改 `_exit`  
4. `waitpid` + `WEXITSTATUS`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | `exit` = atexit + fflush + `_Exit` |
| 2 | fork 子（不 exec）用 `_exit` |
| 3 | 退出码仅低 8 位 |
| 4 | atexit LIFO；信号/`_exit` 不跑 |
| 5 | 内核收尸后僵尸等 wait |
| 6 | 用户缓冲 ≠ 内核刷盘 |

---


---

## 参考


- Kerrisk · TLPI Ch25  
- `man 3 exit` · `man 2 _exit` · `man 3 atexit` · `man 3 abort`


---

## 代码示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Ch25 进程终止 — exit/_exit/atexit/on_exit。
 * exit() 调用 atexit 注册的函数 + flush stdio；
 * _exit() 直接进入内核，不清理。
 * 编译: gcc -o ch25_demo ch25_demo.c */

static void cleanup1(void) {
    printf("atexit handler 1 (called by exit, LIFO order)\n");
}

static void cleanup2(void) {
    printf("atexit handler 2 (called first, LIFO)\n");
}

int main(void) {
    /* atexit: 注册退出处理函数，LIFO 顺序调用 */
    atexit(cleanup1);
    atexit(cleanup2);

    printf("Program running...\n");

    /* fork 后子进程继承 atexit 注册 */
    pid_t pid = fork();
    if (pid == 0) {
        printf("Child: calling _exit (skips atexit handlers)\n");
        _exit(0);  /* 不调 atexit，不 flush stdio */
    }

    /* 父进程用 exit() */
    printf("Parent: calling exit (runs atexit handlers + flush stdio)\n");
    exit(0);
    /* 以下不会执行 */
    printf("This line never prints\n");
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
