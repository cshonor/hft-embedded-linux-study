# 4.1 条件变量 condition_variable

> 第 4 章 同步操作 · 下一节：[4.2 future/promise](02-future-promise.md)

## 这节讲什么

`condition_variable` 让线程等待某个条件成立。`wait` 必须带谓词——虚假唤醒会让无谓词版本失效。

## 为什么要学这个（先建立直觉）

C 程序员用 `pthread_cond_wait` 做条件等待：

```c
// C：pthread 条件变量
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
int ready = 0;

// 等待方
pthread_mutex_lock(&mtx);
while (!ready) {  // 必须用 while（虚假唤醒）
    pthread_cond_wait(&cv, &mtx);  // 等待 + 自动释放/重新获取 mtx
}
pthread_mutex_unlock(&mtx);

// 通知方
pthread_mutex_lock(&mtx);
ready = 1;
pthread_cond_signal(&cv);  // 唤醒一个
pthread_mutex_unlock(&mtx);
```

C++ 标准化了条件变量，并用谓词形式封装了 while 循环：

```cpp
// C++：condition_variable + 谓词
std::mutex m;
std::condition_variable cv;
bool ready = false;

// 等待方（一行代替 while 循环）
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return ready; });  // 内部自动处理虚假唤醒

// 通知方
{
    std::lock_guard<std::mutex> lk(m);
    ready = true;
}
cv.notify_one();
```

## 核心用法详解

### 生产者-消费者模式

```cpp
#include <queue>
#include <mutex>
#include <condition_variable>

std::queue<int> task_queue;
std::mutex m;
std::condition_variable cv;
bool done = false;

// 生产者
void producer() {
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lk(m);
            task_queue.push(i);
        }
        cv.notify_one();  // 通知一个消费者
    }
    {
        std::lock_guard<std::mutex> lk(m);
        done = true;
    }
    cv.notify_all();  // 通知所有消费者退出
}

// 消费者
void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return !task_queue.empty() || done; });

        if (task_queue.empty() && done) return;  // 退出

        auto task = task_queue.front();
        task_queue.pop();
        lk.unlock();  // 处理任务时不需要锁

        process(task);
    }
}
```

### wait 的工作原理

```
cv.wait(lk, pred) 的内部逻辑：
1. while (!pred()) {
2.     lk.unlock();        // 释放锁，让生产者能修改共享状态
3.     阻塞等待 notify;     // 线程被 park（让出 CPU）
4.     被唤醒;
5.     lk.lock();          // 重新获取锁
6. }
// 退出循环时：pred() == true 且持有锁

为什么用 while 而不是 if？
- 虚假唤醒：wait 可能在没有 notify 的情况下返回
- 竞争唤醒：多个消费者被 notify_all 唤醒，但只有一个能取到任务
  → 其他消费者醒来后发现 pred() 仍为 false，需要重新 wait
```

### notify_one vs notify_all

```cpp
// notify_one：唤醒一个等待线程（适合单生产者→单消费者）
cv.notify_one();  // 只唤醒一个，减少不必要的唤醒

// notify_all：唤醒所有等待线程（适合广播/状态变更）
cv.notify_all();  // 所有等待线程都被唤醒，检查条件
```

| 通知方式 | 唤醒数量 | 适用场景 | 性能 |
|----------|----------|----------|------|
| `notify_one` | 1 个 | 每次只加一个任务 | 更好（少唤醒） |
| `notify_all` | 全部 | 状态变更影响所有等待者 | 可能"惊群效应" |

## 常见错误（新手踩坑）

### 错误 1：wait 不带谓词

```cpp
// 错误：无谓词 wait——虚假唤醒导致条件判断失效
std::unique_lock<std::mutex> lk(m);
while (!ready) {
    cv.wait(lk);  // 无谓词——可能虚假唤醒
}
// 虽然手动 while 循环可以工作，但容易遗漏

// 正确：带谓词
cv.wait(lk, [&]{ return ready; });  // 内部自动 while 循环
```

### 错误 2：通知前忘记加锁修改共享状态

```cpp
// 错误：不加锁修改 ready——数据竞争
ready = true;
cv.notify_one();
// 问题：wait 方在 wait 内部检查 pred 时，ready 可能还没被看到

// 正确：在锁内修改
{
    std::lock_guard<std::mutex> lk(m);
    ready = true;
}
cv.notify_one();  // 通知可以在锁外
```

### 错误 3：notify 在锁内（性能问题）

```cpp
// 不理想：notify 在锁内——被唤醒的线程立即尝试 lock 但锁还被持有
{
    std::lock_guard<std::mutex> lk(m);
    ready = true;
    cv.notify_one();  // 被唤醒线程立即尝试 lock → 阻塞 → 上下文切换浪费
}

// 更好：notify 在锁外
{
    std::lock_guard<std::mutex> lk(m);
    ready = true;
}
cv.notify_one();  // 锁已释放，被唤醒线程可以立即获取锁
```

**注意**：notify 在锁内不是错误（功能正确），但可能导致额外上下文切换。在锁外 notify 是性能优化。

## 和 C 的区别

| 特性 | C (pthread) | C++ (std) |
|------|-------------|-----------|
| 条件变量 | `pthread_cond_t` | `std::condition_variable` |
| 等待 | `pthread_cond_wait(&cv, &mtx)` | `cv.wait(lk, pred)` |
| 谓词形式 | 手动 while 循环 | 内置谓词参数 |
| 超时等待 | `pthread_cond_timedwait` | `cv.wait_for`/`wait_until` |
| 通知一个 | `pthread_cond_signal` | `cv.notify_one` |
| 通知全部 | `pthread_cond_broadcast` | `cv.notify_all` |
| 必须配合 | `pthread_mutex_t` | `std::unique_lock<std::mutex>` |

## HFT 关联

- **热路径用自旋替代 cv**：`condition_variable` 会 park 线程（让出 CPU），唤醒延迟 ~1-10μs（上下文切换）。HFT 热路径用 `atomic` + 自旋（`yield`/`pause`）避免上下文切换。
- **cv 适合非热路径**：等待行情连接、等待配置加载、等待交易所响应等非延迟敏感场景用 cv。
- **虚假唤醒对 HFT 的影响**：即使虚假唤醒概率低，在微秒级热路径上仍可能导致延迟抖动——带谓词的 wait 确保正确性，自旋避免 park。

## 代码自测

### Q1: 下列代码有什么问题？

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

// 等待方
std::unique_lock<std::mutex> lk(m);
cv.wait(lk);  // 无谓词
if (ready) do_work();

// 通知方
ready = true;
cv.notify_one();
```

<details>
<summary>答案与复习指引</summary>

**两个问题**：
1. `cv.wait(lk)` 无谓词——虚假唤醒时 `ready` 仍为 false，但代码继续执行。
2. 通知方修改 `ready` 不加锁——数据竞争。

修复：
```cpp
// 等待方
cv.wait(lk, [&]{ return ready; });  // 带谓词

// 通知方
{ std::lock_guard<std::mutex> lk(m); ready = true; }
cv.notify_one();
```

复习：wait 必须带谓词，修改共享状态必须在锁内。
</details>

### Q2: 下列代码的输出是什么？

```cpp
std::queue<int> q;
std::mutex m;
std::condition_variable cv;

// 生产者
{
    std::lock_guard<std::mutex> lk(m);
    q.push(42);
}
cv.notify_one();

// 消费者
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return !q.empty(); });
std::cout << q.front();
q.pop();
```

<details>
<summary>答案与复习指引</summary>

**输出 42**（如果生产者在消费者 wait 之前执行）。

但如果消费者先执行 `cv.wait`，然后生产者 push + notify——也能正确唤醒。

关键：谓词 `[&]{ return !q.empty(); }` 保证了即使虚假唤醒也会重新检查条件。

潜在问题：如果生产者在消费者调用 `wait` 之前就 `notify_one`，通知会丢失。但谓词检查会正确处理：`wait` 先检查谓词，如果已满足就不阻塞。

复习：带谓词的 wait 正确处理"先通知后等待"的情况——先检查谓词，已满足则不阻塞。
</details>

### Q3: notify_one 和 notify_all 有什么区别？何时用哪个？

<details>
<summary>答案与复习指引</summary>

- **notify_one**：唤醒一个等待线程。适合"每次只添加一个任务"的场景——一个生产者通知一个消费者。
- **notify_all**：唤醒所有等待线程。适合"状态变更影响所有等待者"的场景——如关闭信号 `done = true`。

性能差异：notify_all 可能导致"惊群效应"——所有线程被唤醒、争抢锁、大部分发现条件不满足又重新 wait。notify_one 更高效但可能遗漏（如果唤醒的线程不是应该处理的那个）。

复习：单生产者→单消费者用 notify_one；广播/状态变更用 notify_all。
</details>

### Q4: 为什么 HFT 热路径用自旋而不是 condition_variable？

<details>
<summary>答案与复习指引</summary>

`condition_variable::wait` 会 park 线程（让出 CPU），唤醒需要：
1. 操作系统调度器将线程从等待队列移到就绪队列
2. 上下文切换（保存/恢复寄存器、TLB、cache）
3. 总延迟 ~1-10μs

HFT 热路径不能容忍微秒级延迟。自旋方案：
```cpp
while (!ready.load(std::memory_order_acquire))
    _mm_pause();  // 或 std::this_thread::yield()
```
延迟 ~10-100ns（取决于等待时间），无上下文切换。

适用条件：等待时间短（< 上下文切换开销），CPU 资源充足。

复习：cv 适合长等待（毫秒+），自旋适合短等待（纳秒~微秒）。HFT 热路径用自旋。
</details>

---

## 参考与延伸

- 下一节：[4.2 future/promise](02-future-promise.md)
- 回到：[第 4 章](README.md)
