# TLPI 第 30 章 — Threads: Thread Synchronization

> 对应目录：`chapter-30-thread-synchronization/`  
> （勿用 `chapter-30-threads-synchronization` — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 不一致）  
> 书名原文：**Threads: Thread Synchronization**  
> ⚠️ **mutex 管互斥；cond 管等待状态变化。** `cond_wait` 必须用 **`while`（谓词）**，防虚假唤醒。勿拷贝 `pthread_mutex_t`。

**优先级**：🔴（多线程正确性核心）  
**前置**：[Ch29 线程导论](../chapter-29-threads-intro/notes.md)  
**后置**：[Ch31 线程安全 / TLS](../chapter-31-thread-safety-tsd/notes.md)

---

## 章节目标

竞态与临界区；`pthread_mutex_*`；`pthread_cond_*` + while 谓词；生产者-消费者；死锁与职责分离。

---

## 30.1 互斥量 `pthread_mutex_t`

| 概念 | |
|------|--|
| 竞态 | 无保护并发读写，结果依赖调度 |
| 临界区 | 须互斥执行的共享访问代码 |
| mutex | 同一时刻至多一线程进入临界区 |

### 初始化

```c
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;   /* 静态 */
pthread_mutex_init(&mtx, &attr);                   /* 动态 */
pthread_mutex_destroy(&mtx);                       /* 须未持锁 */
```

禁止拷贝 mutex；行为未定义。

### API

```c
pthread_mutex_lock / trylock / timedlock / unlock
```

`trylock` 拿不到 → `EBUSY`（正错误码，Pthreads 风格）。

### 类型（attr）

| 类型 | 要点 |
|------|------|
| `NORMAL`（常见默认） | 同线程再 lock → 死锁；乱 unlock → UB |
| `RECURSIVE` | 可嵌套；unlock 次数须匹配 |
| `ERRORCHECK` | 非法操作返回错误；调试用 |
| `ADAPTIVE_NP`（Linux） | 短自旋再睡；短临界区 |

### 死锁

1. 同线程对普通锁二次 lock  
2. 多锁**获取顺序不一致**  

→ **全局统一加锁顺序**。

Demo：[`code/thread_incr_mutex.c`](./code/thread_incr_mutex.c)（对比 Ch29 `thread_race`）

---

## 30.2 条件变量 `pthread_cond_t`

mutex = 互斥访问；cond = **等某个状态成立**。cond **必须**与 mutex 一起用。

```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_wait / timedwait / signal / broadcast
```

### `wait` 语义

原子：放锁 → 睡 → 醒后**再拿锁**再返回。避免「先 signal、后 wait」丢唤醒（在持锁改状态 + wait 的正确用法下）。

### 谓词必须 `while`

存在**虚假唤醒**；也可能 broadcast 后条件仍不满足：

```c
pthread_mutex_lock(&mtx);
while (!predicate)
    pthread_cond_wait(&cond, &mtx);
/* use shared state */
pthread_mutex_unlock(&mtx);
```

| | |
|--|--|
| `signal` | 唤醒至少一个等待者 |
| `broadcast` | 唤醒全部（多等待者 / 不确定谁该醒） |

### 生产者-消费者

mutex 保护缓冲与计数；空/满时 `wait`；变更后 `signal`/`broadcast`。

Demo：[`code/prod_condvar.c`](./code/prod_condvar.c)

---

## 30.3 职责与易错

| 工具 | 职责 |
|------|------|
| mutex | 保护**状态读写** |
| cond | 等待**状态变化** |

1. wait 不持锁 / 用 `if` 判条件  
2. 非持有者 unlock  
3. 递归锁 unlock 次数不对  
4. 多锁无序 → 死锁  
5. 以为 cond「存信号」：wait 前乱 signal 会丢（须在持锁下改谓词再 signal）  

---

## 实验清单

1. 无锁累加错误（Ch29 `thread_race`）  
2. mutex 修复  
3. （选）`trylock`  
4. while vs if  
5. 生产-消费  
6. （选）多锁死锁  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | 临界区用 mutex |
| 2 | cond 永远配 mutex |
| 3 | `while (!pred) wait` |
| 4 | wait = 放锁睡，醒再加锁 |
| 5 | 多锁统一顺序防死锁 |
| 6 | mutex 管状态，cond 管等待 |

---

## 参考

- Kerrisk · TLPI Ch30  
- `man 3 pthread_mutex_lock` · `man 3 pthread_cond_wait`
