# std::counting_semaphore / binary_semaphore

## 信号量

```cpp
#include <semaphore>

// counting_semaphore：计数信号量
std::counting_semaphore<8> sem(3);  // 最多 8 个等待，初始 3 个许可

// 获取许可
sem.acquire();  // 计数减 1，如果为 0 则阻塞

// 释放许可
sem.release();  // 计数加 1，唤醒一个等待者

// 尝试获取（非阻塞）
if (sem.try_acquire()) {
    // 获取成功
} else {
    // 没有许可
}

// 带超时
if (sem.try_acquire_for(100ms)) {
    // 超时内获取成功
}
```

## binary_semaphore

```cpp
// binary_semaphore：二值信号量（类似 mutex）
std::binary_semaphore sem(1);  // 初始 1（可用）

sem.acquire();  // 获取（锁定）
// 临界区
sem.release();  // 释放（解锁）

// 与 mutex 的区别：
// - semaphore 不可重入（同线程多次 acquire 会死锁）
// - semaphore 可以在不同线程 acquire/release
// - semaphore 可以用于"通知"模式（一个 release 唤醒一个 acquire）
```

## 生产者-消费者模式

```cpp
std::counting_semaphore<10> items(0);    // 有多少产品
std::counting_semaphore<10> slots(10);   // 有多少空位

void producer() {
    while (true) {
        auto product = produce();
        slots.acquire();    // 等待空位
        buffer.push(product);
        items.release();    // 通知有产品
    }
}

void consumer() {
    while (true) {
        items.acquire();    // 等待产品
        auto product = buffer.pop();
        slots.release();    // 通知有空位
        consume(product);
    }
}
```

## HFT 应用

```cpp
// 限制并发订单数
std::counting_semaphore<100> order_limit(100);

void place_order() {
    order_limit.acquire();  // 最多 100 个未完成订单
    // 发送订单
    // 订单完成后
    order_limit.release();
}

// 通知模式：行情线程通知策略线程
std::binary_semaphore tick_ready(0);

void market_thread() {
    while (true) {
        auto tick = recv_tick();
        push_tick(tick);
        tick_ready.release();  // 通知策略线程
    }
}

void strategy_thread() {
    while (true) {
        tick_ready.acquire();  // 等待新 tick
        auto tick = pop_tick();
        process(tick);
    }
}
```

## 自测题

1. `counting_semaphore` 和 `binary_semaphore` 的区别？
2. semaphore 和 mutex 的区别？
3. semaphore 为什么可以跨线程 acquire/release？
4. 生产者-消费者模式如何用信号量实现？
5. HFT 中如何用信号量限制并发订单数？

<details>
<summary>参考答案</summary>

1. `std::counting_semaphore<Max>`：维护一个非负计数，`acquire()` 使计数减 1（为 0 时阻塞），`release(n)` 使计数加 n（默认加 1），模板参数 `Max` 是计数的上限（编译期常量，决定内部用什么整型）。
`std::binary_semaphore` 就是 `std::counting_semaphore<1>` 的别名：计数只能是 0 或 1，语义上是一个"**信号/通知**"（release 表示事件发生，acquire 表示等待事件）。
一句话：counting 用于"**限量**"（限制同时进入的数量），binary 用于"**通知**"。
2. 最根本的区别是**所有权**：
   - `mutex` 有所有权——谁 `lock()` 就必须由同一线程 `unlock()`，跨线程 unlock 是未定义行为；它保护**临界区**。
   - `semaphore` **没有所有权**——任意线程都可以 `release()`，不要求"谁 acquire 谁 release"；它表达的是"**资源许可计数**"或"**事件通知**"。
另外 mutex 只有"锁/未锁"两态，而 counting_semaphore 的计数可以大于 1（表示多个同类资源）。
所以：保护共享数据用 mutex；限流、配对通知（生产者-消费者）用 semaphore。
3. 因为它的语义是"**计数**"而不是"所有权"：`release()` 只是把计数加一（并唤醒一个等待者），标准并不记录是谁 acquire 的，也不要求 release 者与 acquire 者是同一线程。
这正是它能做"线程 A 生产后 `release()`、线程 B `acquire()` 后消费"的原因——release 的含义是"我释放了一个资源/发出了一个信号"，而不是"我归还了我持有的锁"。
这也是它不能当互斥锁用的原因：没有所有权就没有"互斥"的保护语义，误用很容易写出同时进入临界区的代码。
4. 用**两个信号量**分别跟踪"空位数"和"产品数"：
```cpp
constexpr std::size_t CAP = 16;
std::counting_semaphore<CAP> slots(CAP);   // 初始：CAP 个空位
std::counting_semaphore<CAP> items(0);     // 初始：0 个产品

void producer() {
    while (true) {
        auto p = produce();
        slots.acquire();      // 等一个空位
        buffer.push(p);
        items.release();      // 产品 +1
    }
}
void consumer() {
    while (true) {
        items.acquire();      // 等一个产品
        auto p = buffer.pop();
        slots.release();      // 空位 +1
        consume(p);
    }
}
```
（`buffer.push/pop` 本身还需另配 mutex 保护——信号量只负责"配额与通知"，不负责互斥。）
5. 用一个初值等于上限的 `counting_semaphore` 当"并发配额"：
```cpp
std::counting_semaphore<100> order_limit(100);   // 最多 100 个未完成订单

void place_order() {
    order_limit.acquire();      // 占一个名额（占满则阻塞/等待）
    send_order();               // ... 订单成交或撤单后
    order_limit.release();      // 归还名额
}
```
模板参数 `100` 是编译期上限（可选更大的最小类型以省空间），构造参数 `100` 是初始可用名额。这样就能在**不引入复杂排队逻辑**的前提下硬性限制在途订单数，作为风控/流量闸门。

</details>
