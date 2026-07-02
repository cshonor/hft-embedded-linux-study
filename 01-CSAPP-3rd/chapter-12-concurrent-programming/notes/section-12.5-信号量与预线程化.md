## 12.5 用信号量同步线程

### 12.5.1 进度图 (Progress Graphs)

- 指令级 **轨迹** — 合法交错必须满足 **happens-before** 边
- 两个线程对共享变量 **读-改-写** 无互斥 → 出现 **不安全区**

### 12.5.2 信号量 (Semaphores)

```c
sem_t sem;
sem_init(&sem, 0, 1);   // 初值 1 = 二进制锁
sem_wait(&sem);         // P：减 1，为 0 则阻塞
sem_post(&sem);         // V：加 1，唤醒等待者
```

- **计数信号量** — 初值 = 可用资源数（空槽、连接数）

### 12.5.3 互斥 (Mutual Exclusion)

```c
sem_wait(&mutex);
/* 临界区 */
sem_post(&mutex);
```

- 等价于 **mutex**；POSIX 还有 `pthread_mutex_t`

### 12.5.4 调度共享资源 — 生产者-消费者

```c
// 空槽 sem_empty，满槽 sem_full，互斥 sem_mutex
sem_wait(&sem_empty);
sem_wait(&sem_mutex);
/* 放入缓冲区 */
sem_post(&sem_mutex);
sem_post(&sem_full);
```

- **有界缓冲区** — HFT **SPSC/MPSC 无锁队列** 的生产者-消费者抽象来源

### 12.5.5 综合：预线程化并发服务器 (Prethreading)

```
主线程：accept → 把 connfd 放入缓冲区
工作线程池：sem_wait → 取 connfd → echo → 循环
```

| 模式 | 行为 |
|------|------|
| **每连接一线程** | accept 后现 `create` |
| **预线程化** | 固定 N 个 worker 等任务 — **控线程数、减创建开销** |

**HFT：** 网关常用 **固定大小线程池** 或 **每核一个 reactor**；任务队列用 **无锁 ring buffer** 替代 `sem`+全局锁（延迟敏感路径）。

→ [12-HFT](../../../15-HFT-Low-Latency-Practice/) · [13-Systems-Performance Ch6 CPU](../../../13-Systems-Performance-2nd/chapter-06-cpus/)

---

← [本章导读](../README.md)
