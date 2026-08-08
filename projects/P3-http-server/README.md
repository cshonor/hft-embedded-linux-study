# P3 — 并发 HTTP Server（C → C++ 重写）

> 先用 C + epoll + 线程池写一个并发 HTTP server，再用 Modern C++ 重写一遍，亲手感受 RAII/模板/移动语义怎么让代码更安全又不损性能。

## 项目目标

把"用户态系统编程"和"C++ 工程化"两条线在一个项目里打通。同一个功能写两遍，对比点不是"哪个快"，而是**资源管理、错误处理、类型安全**的代际差异。

## 交付物

### Version A：C 版

- [ ] 基于 `socket` + `listen` + `accept` 的 TCP server
- [ ] `epoll` 多路复用（ET 模式 + 非阻塞 fd）
- [ ] 线程池（固定 worker 数，任务队列 + 互斥锁/条件变量）
- [ ] HTTP/1.1 请求解析（GET/POST、Header、Content-Length）
- [ ] 静态文件服务（mime 类型、目录索引）
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
| [`04` linux-userspace-api](../../04-linux-userspace-api/) | TLPI：socket、epoll、线程、信号、mmap |
| [`05` os-from-scratch](../../05-os-from-scratch/) | 自制 OS 的 syscall/中断/调度直觉 |
| [`06` cpp](../../06-cpp/) | Modern C++（M1）+ 并发（M2）+ 对象模型 |

## 前置

[P2](../P2-shell-malloc/)（进程/内存模型过关）。

## 学习目标

- epoll LT vs ET 的语义差异与陷阱
- 线程池的任务窃取/唤醒/优雅关闭
- RAII 如何消灭一类资源泄漏 bug
- 移动语义在热路径减少拷贝的真实场景
- C 风格错误处理 vs C++ `optional`/异常的取舍

## 里程碑

1. **M1** C 版单线程 epoll echo server
2. **M2** C 版加线程池 + HTTP 解析
3. **M3** C 版静态文件服务跑通（用 ab/wrk 压测）
4. **M4** C++ 版 RAII fd + 线程池模板
5. **M5** C++ 版功能对齐 C 版，对比代码量/安全性

## 参考模块

- [04-linux-userspace-api/](../../04-linux-userspace-api/) — TLPI Ch58-63（socket）、Ch63（epoll）、Ch29-30（线程）
- [05-os-from-scratch/](../../05-os-from-scratch/) — mikanos/thirty-days-os：syscall 与中断的内核侧
- [06-cpp/](../../06-cpp/) — Effective Modern C++、Cpp-Concurrency、Cpp-Object-Model

## 压测工具

- `ab -n 10000 -c 100 http://localhost:8080/`
- `wrk -t4 -c100 -d10s http://localhost:8080/`
