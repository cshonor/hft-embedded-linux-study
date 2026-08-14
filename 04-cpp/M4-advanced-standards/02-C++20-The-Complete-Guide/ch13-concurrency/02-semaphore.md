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
