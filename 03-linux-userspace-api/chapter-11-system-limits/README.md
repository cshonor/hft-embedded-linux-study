# TLPI 第 11 章 — System Limits and Options

**优先级**：🟡→🔴（可移植 / 路径 / 管道缓冲）  
**前置**：[Ch10 Times and Dates](../chapter-10-time/README.md)（`_SC_CLK_TCK` ↔ `times()`）  
**后置**：[Ch12 System and Process Information](../chapter-12-system-process-info/README.md) · [Ch15 文件属性](../chapter-15-file-attributes/README.md) · [Ch44 管道](../chapter-44-pipes-fifos/README.md) · [Ch36 资源限制](../chapter-36-process-resources/README.md)

---

## 小节目录

- [11.1 基础概念](notes/11.1-system-limits.md)
- [11.2 三大查询 API](notes/11.2-runtime-limits.md)
- [11.3 编译期：`<limits.h>` / `<unistd.h>`](notes/11.3-file-related-limits.md)
- [11.4 常用限制速览](notes/11.4-indeterminate-limits.md)
- [11.5 Indeterminate：工程策略](notes/11.4-indeterminate-limits.md)
- [11.6 选项检测（Feature Options）](notes/11.5-system-options.md)
- [11.7 高频易错](notes/11.7-exercises.md)

---

## 章节目标


解决可移植性：**不要写死** `MAX_PATH` 之类；掌握 SUSv3 限制与 POSIX 可选特性；会用 `sysconf` / `pathconf` / `fpathconf`；分清三类限制与「错误 vs 不确定」；为文件、管道、IPC 写健壮代码。

---


---

## 与前后章


| 章 | 关联 |
|----|------|
| Ch10 | `_SC_CLK_TCK` ↔ `times()` |
| Ch12 | `/proc`、硬件信息 — 限制信息的另一来源 |
| Ch15+ 文件 | `NAME_MAX` / `PATH_MAX` |
| 管道 / FIFO | `_PC_PIPE_BUF` 原子写边界 |
| Ch36 | `RLIMIT_NOFILE` ↔ `_SC_OPEN_MAX` |

---


---

## 练习


1. 封装安全 `sysconf`：区分错误 / indeterminate；批量打印常用限制  
2. 对不同目录 `pathconf(_PC_NAME_MAX)`（如 `/` vs `/tmp`）  
3. 静态 `PATH_MAX` vs 动态扩容  
4. `getconf` 与程序结果交叉验证  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 三类限制：恒定 / 路径相关 / 可增 |
| 2 | 全局 → `sysconf`；文件系统 → `fpathconf` 优先 |
| 3 | `-1`：先清 `errno`，再分错误 vs indeterminate |
| 4 | `_SC_*` / `_PC_*` 命名 |
| 5 | `_SC_CLK_TCK` ≠ HZ；`PIPE_BUF` 跟文件系统走 |
| 6 | 编译期宏不够，运行时再查 |

---


---

## 参考


- Kerrisk · TLPI Ch11  
- `man 3 sysconf` · `man 3 fpathconf` · `man 1 getconf` · `man 7 posixoptions`


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

/* Ch11 系统限制 — sysconf/pathconf/confstr 获取运行时限制。
 * 编译: gcc -o ch11_demo ch11_demo.c */

int main(void) {
    /* sysconf: 系统级限制 */
    printf("_SC_OPEN_MAX (max fd per process): %ld\n", sysconf(_SC_OPEN_MAX));
    printf("_SC_CLK_TCK (clock ticks/sec):     %ld\n", sysconf(_SC_CLK_TCK));
    printf("_SC_PAGESIZE:                      %ld\n", sysconf(_SC_PAGESIZE));
    printf("_SC_NPROCESSORS_ONLN:              %ld\n", sysconf(_SC_NPROCESSORS_ONLN));

    /* pathconf: 文件/路径相关限制 */
    printf("\n_PC_NAME_MAX (for /tmp):  %ld\n", pathconf("/tmp", _PC_NAME_MAX));
    printf("_PC_PATH_MAX (for /tmp):  %ld\n", pathconf("/tmp", _PC_PATH_MAX));
    printf("_PC_LINK_MAX (for /tmp):  %ld\n", pathconf("/tmp", _PC_LINK_MAX));

    /* 编译期常量 */
    printf("\nINT_MAX = %d\n", INT_MAX);
    printf("LONG_MAX = %ld\n", LONG_MAX);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
