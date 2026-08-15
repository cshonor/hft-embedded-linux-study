# P3 — 并发 HTTP Server（C → C++ 重写）

> 先用 C + epoll + 线程池写一个并发 HTTP server，再用 Modern C++ 重写一遍，亲手感受 RAII/模板/移动语义怎么让代码更安全又不损性能。
> **做法：项目驱动，[`03`](../../03-linux-userspace-api/) / [`12`](../../04.5-network-sockets/) / [`04`](../../04-cpp/) 笔记当字典——先上路，卡住再查。**

---

## 核心理念

跟 P2 一样——不要先读完 TLPI 全书再开做。翻一眼标题知道 epoll 是什么、socket 四件套是什么，直接写代码。

同一个 server 写两遍，对比点不是"哪个快"，而是**资源管理、错误处理、类型安全**的代代差异。

---

## 实现指南

| Part | 内容 | 建议时间 | 可运行工程 |
|------|------|----------|------------|
| [Part A：C 版](./Part-A-c-server.md) | epoll echo → HTTP 解析 → 线程池 → 静态文件 + 压测 | 3-4 小时 | [`part-a-c-server/`](./part-a-c-server/) `make` → `./echo_server` (:8080) |
| [Part B：C++ 重写](./Part-B-cpp-rewrite.md) | RAII fd → 线程池模板 → string_view/optional → 代码对比 | 3-4 小时 | [`part-b-cpp-rewrite/`](./part-b-cpp-rewrite/) `make` → `./echo_server` (:8081) |

**建议顺序**：先完成 Part A 全部 4 个 Phase，再做 Part B。Part B 是在 A 的基础上重写，不是从头开始。

---

## 交付物

### Version A：C 版

- [ ] 基于 `socket` + `listen` + `accept` 的 TCP server
- [ ] `epoll` 多路复用（ET 模式 + 非阻塞 fd）
- [ ] 线程池（固定 worker 数，任务队列 + 互斥锁/条件变量）
- [ ] HTTP/1.1 请求解析（GET/POST、Header、Content-Length）
- [ ] 静态文件服务（mime 类型、`sendfile` 零拷贝）
- [ ] 连接超时关闭

### Version B：C++ 重写版

- [ ] RAII 封装 fd（析构即 `close`，杜绝泄漏）
- [ ] `std::unique_ptr` / `std::shared_ptr` 管理连接对象
- [ ] 模板化线程池（任意可调用任务）
- [ ] 移动语义传递请求对象，零拷贝意图
- [ ] `std::string_view` 解析请求行
- [ ] `std::optional` / 错误码替代裸指针返回

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`03` linux-userspace-api](../../03-linux-userspace-api/) | TLPI：socket、epoll、线程、信号、mmap |
| [`P9` os-from-scratch](../../projects/P9-os-from-scratch/) | 自制 OS 的 syscall/中断/调度直觉 |
| [`04` cpp](../../04-cpp/) | Modern C++（M1）+ 并发（M2）+ 对象模型 |

## 前置

[P2](../P2-shell-malloc/)（进程/内存模型过关）。

## 里程碑

1. **M1** C 版单线程 epoll echo server → [Part A Phase 1](./Part-A-c-server.md)
2. **M2** C 版加线程池 + HTTP 解析 → [Part A Phase 2-3](./Part-A-c-server.md)
3. **M3** C 版静态文件服务跑通（用 ab/wrk 压测）→ [Part A Phase 4](./Part-A-c-server.md)
4. **M4** C++ 版 RAII fd + 线程池模板 → [Part B Phase 1-2](./Part-B-cpp-rewrite.md)
5. **M5** C++ 版功能对齐 C 版，对比代码量/安全性 → [Part B Phase 3-4](./Part-B-cpp-rewrite.md)

## 参考模块

- [03-linux-userspace-api/](../../03-linux-userspace-api/) — TLPI Ch56（socket）、Ch63（epoll）、Ch29-30（线程）
- [04.5-network-sockets/](../../04.5-network-sockets/) — UNP、PNP epoll 实战
- [04-cpp/](../../04-cpp/) — Effective Modern C++、Cpp-Concurrency、Cpp-Object-Model

## 压测工具

```bash
ab -n 10000 -c 100 http://localhost:8080/
wrk -t4 -c100 -d10s http://localhost:8080/
```

## 状态

⬜ 未开始 → 建议先把 Part A Phase 1 的 echo server 跑起来（30 分钟）。

← [projects 总览](../README.md) · [03 模块](../../03-linux-userspace-api/) · [04 模块](../../04-cpp/)
