## 12.5 用信号量同步线程

> ↔ [Hennessy §5.5 同步基础](../../../03-computer-architecture/chapter-05-thread-level-parallelism/notes/section-5.5-同步基础.md)


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

→ [12-HFT](../../../21-hft-engineering/) · [14-Systems-Performance Ch6 CPU](../../../19-systems-performance/chapter-06-cpus/)

### 常见陷阱
1. **sem_wait/sem_post 顺序不能反** — 生产者先 wait(empty) 再 wait(mutex)，反了会死锁（持有 mutex 等 empty）
2. **生产者-消费者需要三个信号量** — mutex（互斥）+ empty（空槽数）+ full（满槽数），缺一不可
3. **预线程化是线程池的思想来源** — 固定 N 个 worker 等任务，避免每连接创建/销毁线程的开销

### 自测题

<details>
<summary>Q1: 信号量的 P（wait）和 V（post）操作分别做什么？</summary>

P（sem_wait）：信号量值减 1，如果结果 < 0 则阻塞等待。V（sem_post）：信号量值加 1，如果有等待者则唤醒一个。P/V 是原子操作。

</details>

<details>
<summary>Q2: 生产者-消费者模式中三个信号量各起什么作用？顺序能否调换？</summary>

mutex：保护缓冲区互斥访问。empty：空槽数，生产者 wait（有空槽才能放）。full：满槽数，消费者 wait（有数据才能取）。顺序不能反：生产者必须先 wait(empty) 再 wait(mutex)，否则持有 mutex 等 empty 会死锁。

</details>

<details>
<summary>Q3: 预线程化（prethreading）和每连接一线程有什么区别？</summary>

每连接一线程：accept 后 pthread_create，连接结束 pthread_exit，频繁创建/销毁。预线程化：启动时创建固定 N 个 worker 线程，主线程 accept 后将 connfd 放入任务队列，worker 取出处理。避免创建/销毁开销，控制线程数。

</details>

<details>
<summary>Q4: HFT 为什么用无锁队列替代信号量+全局锁？</summary>

信号量+全局锁有内核态切换开销（sem_wait 阻塞时 syscall），延迟不确定（被调度时机不可控）。无锁 SPSC 队列用原子操作（CAS/fence），纯用户态，延迟确定（纳秒级），适合热路径。

</details>
---

← [本章导读](../README.md)
