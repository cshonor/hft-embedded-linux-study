# TLPI 第 19 章 — Monitoring File Events

**优先级**：🔴（热重载、配置监视、构建工具、日志跟随）  
**前置**：[Ch18 Directories and Links](../chapter-18-directories-links/notes.md)  
**后置**：[Ch20 Signals](../chapter-20-signals-fundamentals/notes.md) · [Ch63 多路 I/O](../chapter-63-alternative-io/notes.md)

---

## 小节目录

- [19.1 技术演进](./notes/19.1-section-19-1.md)
- [19.3 `struct inotify_event`](./notes/19.3-struct-inotifyevent.md)
- [19.4 常用 mask](./notes/19.4-mask.md)
- [19.5 关键行为](./notes/19.5-section-19-5.md)
- [19.6 典型流程](./notes/19.6-section-19-6.md)

---

## 章节目标


掌握 inotify 实例 / watch / 事件掩码；理解目录 vs 文件监控差异、变长事件缓冲、溢出与 watch 自动删除；能与 `poll`/`epoll` 搭配；规避软链接、移动 cookie、非递归等陷阱。

---


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


---

## 练习


1. 监控目录：打印 create/delete/modify  
2. epoll + 非阻塞 inotify  
3. （选）子目录创建时动态 add_watch  
4. （选）压测溢出  
5. 移动文件观察 cookie  

---


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


---

## 参考


- Kerrisk · TLPI Ch19  
- `man 7 inotify` · `man 2 inotify_init` · `man 2 inotify_add_watch` · `inotifywait(1)`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
