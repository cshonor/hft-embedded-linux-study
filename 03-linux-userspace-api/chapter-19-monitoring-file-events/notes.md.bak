# TLPI 第 19 章 — Monitoring File Events

> 对应目录：`chapter-19-monitoring-file-events/`  
> 书名原文：**Monitoring File Events**  
> ⚠️ **inotify = Linux 专属**（非 POSIX）。内核事件通知；**不递归**；必查 `IN_Q_OVERFLOW`。

**优先级**：🔴（热重载、配置监视、构建工具、日志跟随）  
**前置**：[Ch18 Directories and Links](../chapter-18-directories-links/notes.md)  
**后置**：[Ch20 Signals](../chapter-20-signals-fundamentals/notes.md) · [Ch63 多路 I/O](../chapter-63-alternative-io/notes.md)

---

## 章节目标

掌握 inotify 实例 / watch / 事件掩码；理解目录 vs 文件监控差异、变长事件缓冲、溢出与 watch 自动删除；能与 `poll`/`epoll` 搭配；规避软链接、移动 cookie、非递归等陷阱。

---

## 19.1 技术演进

| 方案 | 评价 |
|------|------|
| 轮询 `readdir`/`stat` | 慢、费资源 |
| **dnotify** | 信号驱动、笨重，已淘汰 |
| **inotify** | 推荐 |
| fanotify | 可拦截访问；审计进阶，本章不展开 |

---

## 19.2 核心 API

```c
#include <sys/inotify.h>

int inotify_init(void);
int inotify_init1(int flags);   /* IN_NONBLOCK | IN_CLOEXEC 推荐 */

int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
int inotify_rm_watch(int fd, int wd);
```

对 inotify **fd** 做 `read()` 取事件；该 fd 可进 `select`/`poll`/`epoll`。

---

## 19.3 `struct inotify_event`

```c
struct inotify_event {
    int      wd;
    uint32_t mask;
    uint32_t cookie;   /* 关联 IN_MOVED_FROM / IN_MOVED_TO */
    uint32_t len;
    char     name[];   /* 变长；监控目录时多为子名 */
};
```

| 监控对象 | `name` |
|----------|--------|
| 普通文件 | 通常空 |
| 目录 | 子项事件时常有子文件名 |

一次 `read` 可能含**多个**变长事件；缓冲要够大，按 `sizeof(struct inotify_event) + NAME_MAX + 1` 倍数预留。

---

## 19.4 常用 mask

### 变更

`IN_ACCESS` · `IN_MODIFY` · `IN_ATTRIB` · `IN_CLOSE_WRITE` · `IN_CLOSE_NOWRITE` · `IN_OPEN`

### 创建 / 删除 / 移动

| 掩码 | 含义 |
|------|------|
| `IN_CREATE` / `IN_DELETE` | 目录内建/删 |
| `IN_DELETE_SELF` / `IN_MOVE_SELF` | 被监控对象自身删/改名 |
| `IN_MOVED_FROM` / `IN_MOVED_TO` | 移出/移入；**同 cookie** 配对 |

### 添加 watch 时的辅助

`IN_DONT_FOLLOW` · `IN_ONESHOT` · `IN_ONLYDIR`

### 内核通知

| 掩码 | 含义 |
|------|------|
| `IN_IGNORED` | watch 已失效（删/卸等） |
| `IN_Q_OVERFLOW` | **队列溢出，丢事件** |
| `IN_UNMOUNT` | FS 卸载 |

---

## 19.5 关键行为

1. **不递归**：只看直接子项；子树要自建 watch，并在 `IN_CREATE` 目录时动态 `add_watch`  
2. watch 文件 vs 目录：见上表；目录自身 `IN_ATTRIB` 常无 `name`  
3. 默认跟随软链接；`IN_DONT_FOLLOW` 盯链接本身  
4. watch 自动没：inode 删除、卸载、`IN_ONESHOT`；主动 `rm_watch`  
5. 溢出：读太慢 → `IN_Q_OVERFLOW`；调 `/proc/sys/fs/inotify/max_user_instances|max_user_watches|max_queued_events`

---

## 19.6 典型流程

1. `inotify_init1(IN_NONBLOCK | IN_CLOEXEC)`  
2. `inotify_add_watch`  
3. fd 进 epoll/poll  
4. 可读则 `read` 一批事件并解析  
5. 新子目录 → 再 `add_watch`（伪递归）  

Demo：[`code/inotify_dir.c`](./code/inotify_dir.c) · [`code/inotify_epoll.c`](./code/inotify_epoll.c)

---

## 19.7 速查：陷阱

| # | 陷阱 |
|---|------|
| 1 | 以为天然递归 |
| 2 | 跨目录移动：源 `MOVED_FROM`、目标 `MOVED_TO` |
| 3 | 硬链：仅最后一链 unlink 才 `DELETE_SELF`；中间只是父目录 `IN_DELETE` |
| 4 | 改名监控对象自身 → `IN_MOVE_SELF` |
| 5 | 缓冲太小 → 截断/解析错乱 |
| 6 | 忽略 `IN_Q_OVERFLOW` → 状态不一致 |
| 7 | NFS 远端变更通常**看不见** |

---

## 练习

1. 监控目录：打印 create/delete/modify  
2. epoll + 非阻塞 inotify  
3. （选）子目录创建时动态 add_watch  
4. （选）压测溢出  
5. 移动文件观察 cookie  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | Linux 专属；fd + `read` 取变长事件 |
| 2 | **不递归**；要自管子目录 watch |
| 3 | 目录事件才有子 `name`；移动用 cookie |
| 4 | 必处理 `IN_Q_OVERFLOW` / `IN_IGNORED` |
| 5 | 生产搭配 epoll/poll |
| 6 | 非 POSIX；NFS 远端无效 |

---

## 参考

- Kerrisk · TLPI Ch19  
- `man 7 inotify` · `man 2 inotify_init` · `man 2 inotify_add_watch` · `inotifywait(1)`
