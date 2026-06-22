## 12.3–12.4 线程与共享变量

### 12.3 基于线程的并发编程

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

### 12.4 多线程程序中的共享变量

#### 12.4.1 线程内存模型

- 各线程有 **私有栈**；**堆、全局、静态** 共享
- **`register` 变量** 语义上私有，编译器可能优化到寄存器 — 别靠「看起来私有」

#### 12.4.2 将变量映射到内存

| 存储类 | 可见性 |
|--------|--------|
| 自动（栈） | 通常仅本线程（除非传指针） |
| 静态 / 全局 | 所有线程 |
| 堆 | 所有持有指针的线程 |

#### 12.4.3 共享变量

- **无同步的并发读写** → 未定义行为 / **race**
- HFT：订单簿、持仓 map、统计计数器 — 必须 **mutex / 原子 / 单写者队列**

**伪共享：** 两线程写同一 **cache line** 不同变量 → 行乒乓（→ [Ch 6](../../chapter-06-memory-hierarchy/) `alignas(64)` 填充）

---

← [本章导读](../README.md)
