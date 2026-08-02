# TLPI 第 11 章 — System Limits and Options

> 对应目录：`chapter-11-system-limits/`  
> 书名原文：**System Limits and Options**  
> ⚠️ **禁止硬编码资源上限**；编译期宏不够时必须运行时查询。

**优先级**：🟡→🔴（可移植 / 路径 / 管道缓冲）  
**前置**：[Ch10 Times and Dates](../chapter-10-time/notes.md)（`_SC_CLK_TCK` ↔ `times()`）  
**后置**：[Ch12 System and Process Information](../chapter-12-system-process-info/notes.md) · [Ch15 文件属性](../chapter-15-file-attributes/notes.md) · [Ch44 管道](../chapter-44-pipes-fifos/notes.md) · [Ch36 资源限制](../chapter-36-process-resources/notes.md)

---

## 章节目标

解决可移植性：**不要写死** `MAX_PATH` 之类；掌握 SUSv3 限制与 POSIX 可选特性；会用 `sysconf` / `pathconf` / `fpathconf`；分清三类限制与「错误 vs 不确定」；为文件、管道、IPC 写健壮代码。

---

## 11.1 基础概念

| 概念 | 含义 |
|------|------|
| **Limits** | 资源 / 路径 / 进程能力上限 |
| **Options** | 标准可选特性（系统可实现或不实现） |

❌ `#define MAX_PATH 256` — 各系统、各文件系统不一致  
✅ 运行时查询上限

SUSv3：每个限制有**最低强制下限**，在 `<limits.h>`，前缀 `_POSIX_`  
例：`_POSIX_ARG_MAX` = 4096 → 所有 POSIX 系统**至少**这么多，实际往往更大。

### 三类限制（核心）

| 类 | 特征 | 查询 | 例 |
|----|------|------|-----|
| **1. 运行时恒定** | 进程生命期内不变；`<limits.h>` 可能未定义 | `sysconf` | `_SC_OPEN_MAX`、`_SC_PAGESIZE` |
| **2. 路径名相关** | **依赖文件系统** / 挂载点 | `pathconf` / `fpathconf` | `NAME_MAX`、`PATH_MAX`、`PIPE_BUF` |
| **3. 运行时可增** | 保证 ≥ 下限，运行中可上调 | 多为 `sysconf` + 资源限制 | 与 Ch36 `rlimit` 相关 |

---

## 11.2 三大查询 API

```c
#include <unistd.h>

long sysconf(int name);                         /* 系统级，不依赖路径 */
long pathconf(const char *pathname, int name);  /* 按路径 */
long fpathconf(int fd, int name);               /* 按 fd，优先 */
```

### 返回值陷阱（必记）

`返回 -1` 有两种含义：

| 条件 | 含义 |
|------|------|
| `errno != 0` | **真正出错**（如 `EINVAL`：name 非法） |
| `errno == 0` | 限制 **indeterminate（不确定）**，不是错误 |

```c
errno = 0;
long v = sysconf(name);
if (v == -1) {
    if (errno == 0)
        /* indeterminate */;
    else
        /* error */;
}
```

### 常量命名

| API | 前缀 | 例 |
|-----|------|-----|
| `sysconf` | `_SC_*` | `_SC_ARG_MAX`、`_SC_OPEN_MAX`、`_SC_PAGESIZE`、`_SC_CLK_TCK` |
| `pathconf`/`fpathconf` | `_PC_*` | `_PC_NAME_MAX`、`_PC_PATH_MAX`、`_PC_PIPE_BUF` |

Shell：`getconf`（封装同类查询）

```bash
getconf ARG_MAX
getconf NAME_MAX /home
```

Demo：[`code/print_limits.c`](./code/print_limits.c)

---

## 11.3 编译期：`<limits.h>` / `<unistd.h>`

| 头文件 | 内容 |
|--------|------|
| `<limits.h>` | `_POSIX_*_MAX` 标准最低下限；部分系统还有实现定义的 `NAME_MAX` 等 |
| `<unistd.h>` | 特性宏：`_POSIX_THREADS`、`_POSIX_REALTIME_SIGNALS` 等 |

> 宏只代表**编译期声明**；裁剪内核 / 容器可能关掉功能 → **可靠做法仍是 `sysconf` 运行时校验**。

---

## 11.4 常用限制速览

### `sysconf(_SC_*)`

| 常量 | 含义 |
|------|------|
| `_SC_ARG_MAX` | `exec` 参数 + 环境变量总字节上限 |
| `_SC_OPEN_MAX` | 进程最大打开 fd 数（Linux 上常对应软限制，见 Ch36） |
| `_SC_PAGESIZE` / `_SC_PAGE_SIZE` | 页大小 |
| `_SC_CLK_TCK` | `times()` 时钟节拍（**≠ 内核 HZ**） |
| `_SC_NGROUPS_MAX` | 补充组最大个数 |
| `_SC_LOGIN_NAME_MAX` | 登录名最大长度 |
| `_SC_THREADS` 等 | 运行时选项检测（是否支持某特性） |

### `pathconf` / `fpathconf(_PC_*)`

| 常量 | 含义 |
|------|------|
| `_PC_NAME_MAX` | 单组件文件名最大长度 |
| `_PC_PATH_MAX` | 相对路径最大长度（注意：绝对路径缓冲不能想当然） |
| `_PC_PIPE_BUF` | 管道**原子写**最大字节 |
| `_PC_NO_TRUNC` | 文件名超长是否报错（非截断） |

---

## 11.5 Indeterminate：工程策略

当 `-1` 且 `errno==0`：

1. 用 `_POSIX_*` **最小值**保守分配  
2. 启发式固定缓冲（如 4096）— 权宜  
3. **动态扩容**（推荐：路径解析、readline 一类）

典型坑：`PATH_MAX` 在部分系统**未定义**；静态大数组仍可能不够或浪费。

---

## 11.6 选项检测（Feature Options）

| 时机 | 方式 |
|------|------|
| 编译期 | `<unistd.h>` 的 `_POSIX_*` |
| 运行期 | `sysconf(_SC_*)`（如 `_SC_THREADS`） |

两者不一致时以**运行时**为准。

---

## 11.7 高频易错

1. **`_SC_CLK_TCK` ≠ 内核 `HZ`**；`times()` 换算秒必须用它；别和 `clock()` 混用。  
2. **`PATH_MAX`** 语义是相对路径一类上限；拼绝对路径勿直接当万能缓冲。  
3. **`PIPE_BUF`** 随文件系统变 → 用 `fpathconf(fd, _PC_PIPE_BUF)`。  
4. **先 `errno=0`** 再调；勿见 `-1` 就当错误。  
5. Linux：`sysconf(_SC_OPEN_MAX)` 常反映**软限制**，可用 `getrlimit`/`setrlimit`（Ch36）。  
6. 特性宏 ≠ 运行时一定可用 → 容器 / 裁剪内核要再查。

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

## 练习

1. 封装安全 `sysconf`：区分错误 / indeterminate；批量打印常用限制  
2. 对不同目录 `pathconf(_PC_NAME_MAX)`（如 `/` vs `/tmp`）  
3. 静态 `PATH_MAX` vs 动态扩容  
4. `getconf` 与程序结果交叉验证  

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

## 参考

- Kerrisk · TLPI Ch11  
- `man 3 sysconf` · `man 3 fpathconf` · `man 1 getconf` · `man 7 posixoptions`
