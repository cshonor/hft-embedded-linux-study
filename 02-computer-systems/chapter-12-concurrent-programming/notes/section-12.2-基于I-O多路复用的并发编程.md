## 12.2 基于 I/O 多路复用的并发编程

> **Ch12 §12.2** · [章导读](../README.md) · 上节 [§12.1 ←](./section-12.1-基于进程的并发编程.md) · 下节 [§12.3 →](./section-12.3-基于线程的并发编程.md)

---

← [本章导读](../README.md)

---

### I/O 多路复用并发模型

- **核心思想：** 单线程同时监听多个 fd，哪个就绪就处理哪个
- **select / poll / epoll：**

| API | 机制 | 限制 | HFT 适用 |
|-----|------|------|----------|
| `select` | 遍历 fd_set，内核检查每个 fd | FD_SETSIZE=1024 | 不用 |
| `poll` | 传 fd 数组，无数量限制 | 仍 O(n) 遍历 | 不用 |
| `epoll` | 内核维护就绪列表，只返回就绪 fd | O(1) 就绪通知 | 标配 |

- **epoll 触发模式：**
  - **LT（水平触发）** — 只要 fd 有数据可读，每次 epoll_wait 都返回（默认）
  - **ET（边缘触发）** — 只在状态变化时通知一次，必须非阻塞 + 读完

**事件驱动服务器骨架：**
```c
epfd = epoll_create1(0);
epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev); // EPOLLIN
while (1) {
    n = epoll_wait(epfd, events, MAXEV, -1);
    for (i = 0; i < n; i++) {
        if (events[i].data.fd == listenfd) { /* accept */ }
        else { /* read/write */ }
    }
}
```

**HFT：** epoll ET + 非阻塞 socket 是低延迟网关标配；但 tick 线程可能用 busy-poll 而非 epoll_wait。

### 常见陷阱
1. **select 有 FD_SETSIZE 限制（默认 1024）** — 连接数超 1024 必须用 poll 或 epoll
2. **epoll 比 select/poll 高效（O(1) vs O(n)）** — 内核维护就绪列表，只返回有事件的 fd，不遍历全部
3. **ET 模式必须非阻塞 + 读完** — 否则可能丢失数据（只通知一次）；LT 模式更安全但可能多一次 epoll_wait

### 自测题

<details>
<summary>Q1: select、poll、epoll 的主要区别？</summary>

select：fd_set 位数组，有 FD_SETSIZE=1024 限制，每次调用遍历所有 fd（O(n)）。poll：用 fd 数组，无数量限制，但仍 O(n) 遍历。epoll：内核维护就绪列表，epoll_wait 只返回就绪 fd（O(1)），适合大量连接。

</details>

<details>
<summary>Q2: epoll 的 LT 和 ET 模式有什么区别？HFT 用哪个？</summary>

LT（水平触发）：fd 有数据就持续通知，直到读完。ET（边缘触发）：状态变化时只通知一次，必须非阻塞读完。HFT 通常用 ET + 非阻塞，减少 epoll_wait 次数；但需小心确保读完所有数据。

</details>

<details>
<summary>Q3: 为什么 I/O 多路复用适合网络服务器但不适合计算密集任务？</summary>

I/O 多路复用解决的是「等待 I/O 时不阻塞」的问题，适合 I/O 密集（网络服务器）。计算密集任务不等待 I/O，单线程多路复用无法利用多核，需要多线程/多进程。

</details>

<details>
<summary>Q4: HFT 网关中 epoll 和 busy-poll 各有什么适用场景？</summary>

epoll：适合管理大量连接（admin API、多客户端），事件驱动，CPU 占用低。busy-poll（SO_BUSY_POLL）：适合 tick 线程，内核持续轮询网卡，延迟最低但 CPU 100% 占用。

</details>

---

← [§12.1 ←](./section-12.1-基于进程的并发编程.md) · [本章导读](../README.md) · [§12.3 →](./section-12.3-基于线程的并发编程.md)
