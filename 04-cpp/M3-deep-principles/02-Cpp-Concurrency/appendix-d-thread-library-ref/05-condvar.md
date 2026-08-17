# D.5 condition_variable 条件变量

> 附录 D · 上一节：[D.4 future 异步结果](04-future.md) · 下一节：[D.6 semaphore 信号量](06-semaphore.md)

## 这节讲什么

`<condition_variable>` 头文件提供条件变量——让线程等待某个条件成立。本节是速查参考——`condition_variable` 的接口、谓词 wait、`notify_one`/`notify_all`。

---

## 核心规则（代码+表格）

### `condition_variable` 接口

| 接口 | 说明 |
|------|------|
| `wait(lk, pred)` | 等待直到 pred 为 true（谓词版） |
| `wait(lk)` | 等待 notify（无谓词，需手动循环） |
| `wait_for(lk, duration, pred)` | 超时等待 |
| `wait_until(lk, time_point, pred)` | 等到时间点 |
| `notify_one()` | 唤醒一个等待者 |
| `notify_all()` | 唤醒所有等待者 |

### 基本用法

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;
int data = 0;

// 消费者
void consumer() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [&]{ return ready; });  // 谓词版：自动处理虚假唤醒
    std::cout << "data: " << data << "\n";
}

// 生产者
void producer() {
    {
        std::lock_guard<std::mutex> lk(m);
        data = 42;
        ready = true;
    }
    cv.notify_one();  // 唤醒一个消费者
}
```

### 谓词版 vs 非谓词版

```cpp
// 谓词版（推荐）：自动循环检查
cv.wait(lk, [&]{ return ready; });
// 等价于：
while (!ready) {
    cv.wait(lk);  // 可能虚假唤醒，需要循环
}

// 非谓词版：必须手动循环
std::unique_lock<std::mutex> lk(m);
while (!ready) {
    cv.wait(lk);  // 释放 lk，等待；被唤醒后重新获取 lk
}
// 虚假唤醒：wait 可能在没有 notify 的情况下返回
// → 必须用 while 循环重新检查条件
```

### 超时等待

```cpp
std::unique_lock<std::mutex> lk(m);
if (cv.wait_for(lk, 500ms, [&]{ return ready; })) {
    // 条件满足
    std::cout << "data: " << data << "\n";
} else {
    // 超时
    std::cout << "timeout\n";
}

// 等到时间点
cv.wait_until(lk, std::chrono::steady_clock::now() + 1s, [&]{ return ready; });
```

### `notify_one` vs `notify_all`

```cpp
// notify_one：唤醒一个等待者（适合单消费者）
cv.notify_one();  // 唤醒一个（如果有多个等待者，选哪个不确定）

// notify_all：唤醒所有等待者（适合多消费者）
cv.notify_all();  // 唤醒所有

// 经验：
// - 单消费者：notify_one（避免惊群）
// - 多消费者：notify_all
// - 不确定：notify_all（安全但可能低效）
```

### 生产者-消费者队列

```cpp
template <typename T>
class ThreadsafeQueue {
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lk(m);
            q.push(std::move(value));
        }
        cv.notify_one();  // 通知一个消费者
    }

    T pop() {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [this]{ return !q.empty(); });  // 等待非空
        T value = std::move(q.front());
        q.pop();
        return value;
    }

    bool try_pop(T& value, auto duration) {
        std::unique_lock<std::mutex> lk(m);
        if (!cv.wait_for(lk, duration, [this]{ return !q.empty(); }))
            return false;
        value = std::move(q.front());
        q.pop();
        return true;
    }
};
```

### `condition_variable_any`

```cpp
// condition_variable_any 可以配合任意锁类型（不只是 unique_lock<mutex>）
std::condition_variable_any cv_any;
std::shared_mutex sm;

void reader() {
    std::shared_lock<std::shared_mutex> slk(sm);  // 读锁
    cv_any.wait(slk, [&]{ return ready; });  // 配合 shared_lock
}
// 但 condition_variable_any 比 condition_variable 慢（内部需要额外同步）
```

---

## 新手要点（和 C 的区别）

- **C 用 `pthread_cond_t` + `pthread_cond_wait`**：功能相同，但 C++ 的谓词版 `wait(lk, pred)` 自动处理虚假唤醒——C 程序员要手动 `while` 循环，容易漏。
- **`unique_lock` 是 cv 的必备**：`condition_variable::wait` 需要 `unique_lock<mutex>`（可临时释放/重新获取）——`lock_guard` 不行。C 程序员用 `pthread_mutex_t` 直接传，C++ 必须用 `unique_lock` 包装。
- **虚假唤醒是真实存在的**：C 程序员可能觉得"虚假唤醒只是理论"——但 POSIX 规范允许它，实际也会发生。C++ 的谓词版 `wait(lk, pred)` 是安全的写法。
- **`notify_one` 的"惊群"问题**：C 程序员可能习惯 `pthread_cond_broadcast`（唤醒所有）——但如果是单消费者，`notify_one` 更高效（不惊群）。C++ 的 `notify_one/notify_all` 对应 `signal/broadcast`。

---

## HFT 关联

- **HFT 热路径不用 condition_variable**：`cv.wait` 竞争时走 futex 系统调用，延迟微秒级——HFT 热路径用无锁队列轮询。
- **`cv` 用于管理面**：HFT 的任务队列、配置更新通知用 `cv`——等待时睡眠不占 CPU。
- **谓词版是必须的**：HFT 代码中如果用 `cv`，一律用谓词版 `wait(lk, pred)`——虚假唤醒在 HFT 系统的高负载下可能更频繁。
- **`notify_one` 用于 SPSC 场景**：HFT 的单消费者任务队列用 `notify_one`——避免惊群开销。

---

## 自测题

1. `condition_variable::wait` 为什么需要 `unique_lock` 而非 `lock_guard`？
2. 谓词版 `wait(lk, pred)` 和非谓词版有什么区别？为什么谓词版更好？
3. 什么是虚假唤醒？如何处理？
4. `notify_one` 和 `notify_all` 各适合什么场景？
5. 为什么 HFT 热路径不用 condition_variable？

---

## 参考与延伸

- 下一节：[D.6 semaphore 信号量](06-semaphore.md)
- 上一节：[D.4 future 异步结果](04-future.md)
- 回到：[附录 D](README.md)
