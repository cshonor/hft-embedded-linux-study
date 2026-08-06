# D.6 semaphore 信号量

> 附录 D · 上一节：[D.5 condition_variable 条件变量](05-condvar.md) · 下一节：[D.7 latch / barrier 屏障](07-latch-barrier.md)

## 这节讲什么

`<semaphore>` 头文件（C++20）提供信号量——比 mutex 更灵活的同步原语。本节是速查参考——计数信号量、二进制信号量（替代 mutex）、以及典型应用。

---

## 核心规则（代码+表格）

### `counting_semaphore` 接口

| 接口 | 说明 |
|------|------|
| `counting_semaphore<N>` | 计数信号量，最大计数 N |
| `release(n=1)` | 计数 +n，唤醒等待者 |
| `acquire()` | 计数 -1（如果为 0 则阻塞） |
| `try_acquire()` | 尝试 -1，非阻塞 |
| `try_acquire_for(duration)` | 超时尝试 |
| `try_acquire_until(time_point)` | 等到时间点 |

### 基本用法

```cpp
#include <semaphore>

// 计数信号量：初始 3，表示 3 个资源
std::counting_semaphore<3> sem(3);

// 线程获取资源
sem.acquire();  // 计数 -1，如果为 0 阻塞
// 使用资源
sem.release();  // 计数 +1，唤醒等待者

// 非阻塞尝试
if (sem.try_acquire()) {
    // 拿到资源
    sem.release();
} else {
    // 无可用资源
}

// 超时
if (sem.try_acquire_for(100ms)) {
    // 拿到
    sem.release();
}
```

### 二进制信号量 = mutex

```cpp
// binary_semaphore 是 counting_semaphore<1> 的别名
std::binary_semaphore sem(1);  // 初始 1

// 用法类似 mutex
sem.acquire();  // 等价于 lock
// 临界区
sem.release();  // 等价于 unlock

// 区别：binary_semaphore 可以在不同线程 acquire/release
// mutex 通常要求同线程 lock/unlock
```

### 应用：资源池

```cpp
// 限制并发连接数
class ConnectionPool {
    std::counting_semaphore<10> sem{10};  // 最多 10 个并发
    std::vector<Connection> connections;
public:
    Connection& acquire() {
        sem.acquire();  // 限制并发数
        return get_free_connection();
    }
    void release(Connection& conn) {
        return_connection(conn);
        sem.release();  // 释放一个名额
    }
};
```

### 应用：线程同步（起步信号）

```cpp
// 所有线程等待起步信号
std::counting_semaphore<1> start_sem(0);  // 初始 0

void worker(int id) {
    // 准备就绪
    start_sem.acquire();  // 等待起步信号
    // 开始工作
    std::cout << "worker " << id << " started\n";
}

// 启动所有线程
std::vector<std::thread> threads;
for (int i = 0; i < 4; ++i)
    threads.emplace_back(worker, i);

std::this_thread::sleep_for(1s);
start_sem.release(4);  // 释放 4 个，唤醒所有等待者

for (auto& t : threads) t.join();
```

### semaphore vs mutex vs condition_variable

| 维度 | semaphore | mutex | condition_variable |
|------|-----------|-------|-------------------|
| 计数 | 支持多资源 | 0/1 | 无 |
| 跨线程释放 | 可以 | 通常同线程 | - |
| 唤醒 | release(n) 唤醒 n 个 | unlock 唤醒 1 个 | notify_one/all |
| 等待 | acquire 阻塞 | lock 阻塞 | wait 阻塞 |
| 用途 | 资源池、同步 | 互斥 | 条件等待 |
| C++ 版本 | C++20 | C++11 | C++11 |

---

## 新手要点（和 C 的区别）

- **C 用 POSIX 信号量 `sem_t`**：`sem_init`/`sem_wait`/`sem_post`——功能和 C++ 的 `counting_semaphore` 相同。但 POSIX 信号量在 macOS 上有限制（named semaphore 更可靠）。C++20 的 `counting_semaphore` 跨平台。
- **"二进制信号量 = mutex"是 C 程序员可能混淆的**：C 程序员可能觉得"信号量和 mutex 差不多"——功能上二进制信号量可以替代 mutex，但语义不同：mutex 要求同线程 lock/unlock，信号量可以跨线程。通常互斥用 mutex，资源计数用信号量。
- **`release(n)` 唤醒多个**：C 的 `sem_post` 只 +1——C++ 的 `release(n)` 可以一次 +n，唤醒 n 个等待者。这在"一次启动多线程"场景很方便。
- **C++20 才有标准信号量**：C 程序员如果之前在 C++ 用信号量，可能用 Boost 或自研（mutex + cv + 计数器）。C++20 终于标准了。

---

## HFT 关联

- **HFT 热路径不用信号量**：`acquire` 竞争时阻塞——HFT 热路径用 atomic 或无锁队列。
- **资源池用信号量**：HFT 的连接池（如交易所连接）用 `counting_semaphore` 限制并发连接数——管理面设施。
- **起步信号**：HFT 系统启动时，各线程准备就绪后等待"开始交易"信号——`counting_semaphore(0)` + `release(N)` 一次唤醒所有线程。
- **`binary_semaphore` 替代 mutex 的场景**：需要跨线程释放锁的场景（如一个线程 acquire、另一个 release）——mutex 不支持，用 `binary_semaphore`。

---

## 自测题

1. `counting_semaphore` 和 `binary_semaphore` 有什么区别？
2. 信号量和 mutex 的语义有什么不同？什么场景用信号量而非 mutex？
3. `release(n)` 做了什么？如何用信号量一次唤醒多个等待者？
4. 如何用信号量实现"资源池"模式？
5. 为什么 HFT 热路径不用信号量？

---

## 参考与延伸

- 下一节：[D.7 latch / barrier 屏障](07-latch-barrier.md)
- 上一节：[D.5 condition_variable 条件变量](05-condvar.md)
- 回到：[附录 D](README.md)
