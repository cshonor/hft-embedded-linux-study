## 12.3 基于线程的并发编程

> **Ch12 §12.3** · [章导读](../README.md) · 上节 [§12.2 ←](./section-12.2-基于I-O多路复用的并发编程.md) · 下节 [§12.4 →](./section-12.4-多线程程序中的共享变量.md)

---

#### 12.3.1 线程执行模型

- **内核线程** — 同一进程内多线程 **共享** 地址空间、fd、堆
- 比进程轻：创建/切换成本低，但需 **同步**

#### 12.3.2–12.3.7 Posix 线程 API

```c
pthread_t tid;
pthread_create(&tid, NULL, thread_func, arg);
pthread_join(tid, &retval);   // 等待结束
pthread_detach(tid);            // 分离，资源由系统回收
pthread_cancel(tid);
pthread_exit(NULL);
```

| API | 要点 |
|-----|------|
| `pthread_create` | 新线程从 `thread_func(arg)` 开始 |
| `pthread_join` | 类似 `waitpid`，收 **retval** |
| `pthread_detach` | 不能再 `join`；避免僵尸线程 |
| 栈大小 / 属性 | `pthread_attr_t` |

- 主线程 `main` 也是线程；`exit` 会结束整个进程

#### 12.3.8 基于线程的并发服务器

```c
while (1) {
    connfd = accept(...);
    pthread_create(&tid, NULL, thread, &connfd);
    pthread_detach(tid);
}
```

- **每连接一线程** — 简单；连接数上千时线程爆炸
- 改进 → **线程池 / 预线程化**（12.5.5）

---

### 常见陷阱
1. **线程共享地址空间，一个线程崩溃全进程崩溃** — 不像进程有隔离保护；线程错误（如栈溢出）影响整个进程
2. **每连接一线程在连接数多时爆炸** — 线程虽比进程轻，但仍有栈开销（默认 8MB/线程），1000 连接 = 8GB 栈空间
3. **pthread_detach 后不能再 join** — 分离线程结束后资源自动回收，但无法获取返回值或等待完成

### 自测题

<details>
<summary>Q1: 线程和进程的核心区别？为什么线程比进程轻？</summary>

线程共享地址空间（代码/堆/fd），进程独立。线程轻因为创建时不复制页表（共享），切换时不需要切换地址空间（不刷 TLB）。

</details>

<details>
<summary>Q2: pthread_create、pthread_join、pthread_detach 分别做什么？</summary>

create：创建新线程，从指定函数开始执行。join：等待线程结束并获取返回值（类似 waitpid）。detach：将线程标记为分离状态，结束后资源自动回收，不能再 join。

</details>

<details>
<summary>Q3: 每连接一线程的模式有什么问题？如何改进？</summary>

问题：1) 线程数随连接数增长，栈空间耗尽；2) 线程切换开销累积；3) 线程太多导致缓存抖动。改进：线程池（固定 N 个 worker），预线程化（提前创建好线程等任务）。

</details>

<details>
<summary>Q4: 主线程调用 exit() 会怎样？和 pthread_exit() 有何区别？</summary>

exit() 终止整个进程（所有线程立即结束）。pthread_exit() 只退出当前线程，其他线程继续运行。主线程从 main 返回等价于调用 exit()，会终止所有线程。

</details>

---

← [§12.2 ←](./section-12.2-基于I-O多路复用的并发编程.md) · [本章导读](../README.md) · [§12.4 →](./section-12.4-多线程程序中的共享变量.md)
