# 25.4 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 机制局限

- 信号上下文切换开销  
- TCP 状态复杂 → **几乎仅适用于 UDP**  

### 2. 并发底线

单线程 + 异步信号处理函数 = 必须用 **`sigprocmask` / `sigsuspend`** 保护共享队列临界区。

### 3. 历史地位

传统 Unix 提升 UDP 接收的偏门手段；现代已被 **epoll / kqueue** 等事件驱动取代（无信号竞态）。

---

## I/O 模型演进（Ch 6 → 25）

```text
阻塞 → 非阻塞轮询 → select/poll → 信号驱动(UDP) → epoll/kqueue
TCP 服务器：select/epoll >> SIGIO
```

---

## 个人学习总结

（待填）
