# 14.9 高级轮询技术

> [Ch 6 select/poll](../../1_BasicFoundation/Chapter06_IO_Select_Poll/study.md)

---

## 核心背景（C10K）

`select`/`poll` 每次 **O(N)** 扫描全部描述符，连接上万时成为瓶颈。

---

## 原书第 3 版重点

### `/dev/poll`（Solaris）

```text
open /dev/poll → write 注册 fd → ioctl(DP_POLL) 阻塞
→ 只返回就绪 fd，无需每次传巨大 fd_set
```

### `kqueue`（FreeBSD）

```text
kqueue() 创建队列 → kevent() 注册/取事件（struct kevent）
```

- 不限套接字：文件变更、进程退出、信号等  
- 性能约 **O(活跃事件)**  

---

## 时代补充（对照现代 Linux）

| API | 要点 |
|-----|------|
| **`epoll`** | Nginx/Redis 常用；**LT** 水平触发 vs **ET** 边缘触发 |
| 共性 | 避免线性扫描全表 fd |

---

## 个人学习总结

（待填）
