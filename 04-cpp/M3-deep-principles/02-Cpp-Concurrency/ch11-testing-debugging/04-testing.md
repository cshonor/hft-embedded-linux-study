# 11.4 并发测试策略

> 第 11 章 · 上一节：[11.3 死锁的检测](03-deadlock.md) · 下一节：[11.5 定位技巧](05-locating.md)

## 这节讲什么

并发测试和串行测试完全不同——不能只验证"结果正确"，要验证"在高并发下也正确"。本节讲压力测试、随机调度测试、组合测试、以及如何设计能暴露竞争的测试用例。

---

## 核心规则（代码+表格）

### 并发测试的层次

| 层次 | 方法 | 目标 |
|------|------|------|
| 单元测试 | 基本功能验证 | 接口正确性 |
| 并发单元测试 | 多线程压测单组件 | 组件级竞争 |
| 集成测试 | 多组件并发协作 | 接口级竞争 |
| 压力测试 | 超高并发+长时间 | 罕见竞争路径 |
| 随机调度 | 随机延迟/让步 | 覆盖更多交错 |

### 压力测试设计

```cpp
// 压力测试：高并发 + 长时间 + 随机性
void stress_test() {
    threadsafe_queue<int> q;
    std::atomic<bool> stop{false};
    std::atomic<int> push_count{0};
    std::atomic<int> pop_count{0};

    // 多生产者
    std::vector<std::thread> producers;
    for (int t = 0; t < 4; ++t) {
        producers.emplace_back([&, t]{
            while (!stop) {
                q.push(t);
                push_count.fetch_add(1);
                // 随机让步，增加交错多样性
                if (rand() % 100 == 0) std::this_thread::yield();
            }
        });
    }

    // 多消费者
    std::vector<std::thread> consumers;
    for (int t = 0; t < 4; ++t) {
        consumers.emplace_back([&]{
            while (!stop) {
                int v;
                if (q.pop(v)) pop_count.fetch_add(1);
                if (rand() % 100 == 0) std::this_thread::yield();
            }
        });
    }

    // 跑 60 秒
    std::this_thread::sleep_for(std::chrono::seconds(60));
    stop = true;
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    // 验证：push 和 pop 的数量关系
    std::cout << "pushed: " << push_count << " popped: " << pop_count << "\n";
    // 队列里剩余 = push - pop
}
```

### 随机调度测试

```cpp
// 在关键点插入随机延迟或让步，增加交错多样性
void randomized_worker(threadsafe_queue<int>& q) {
    for (int i = 0; i < 10000; ++i) {
        q.push(i);
        // 随机选择：继续、让步、睡 1μs
        switch (rand() % 3) {
            case 0: break;  // 继续
            case 1: std::this_thread::yield(); break;  // 让步
            case 2: std::this_thread::sleep_for(std::chrono::microseconds(1)); break;
        }
    }
}
// 这种"随机调度"能覆盖更多线程交错，暴露在正常时序下不触发的竞争
```

### 组合测试

```cpp
// 不同操作组合，暴露交互竞争
void combination_test() {
    threadsafe_map<int, int> m;
    std::vector<std::thread> threads;
    
    // 线程1：交替 get/set
    threads.emplace_back([&]{
        for (int i = 0; i < 100000; ++i) {
            m.set(i % 100, i);
            m.get(i % 100);
        }
    });
    // 线程2：遍历 + 删除
    threads.emplace_back([&]{
        for (int i = 0; i < 1000; ++i) {
            m.clear();
        }
    });
    // 线程3：只读
    threads.emplace_back([&]{
        for (int i = 0; i < 100000; ++i) {
            m.get(i % 100);
        }
    });
    // 组合 get/set/clear 可能暴露迭代器失效等竞争
}
```

### 测试覆盖率

| 覆盖维度 | 说明 |
|----------|------|
| 路径覆盖 | 所有代码路径 |
| 交错覆盖 | 尽可能多的线程交错 |
| 数据覆盖 | 边界值（空、满、单元素） |
| 时间覆盖 | 不同负载下的行为 |
| 组合覆盖 | 不同操作的组合 |

---

## 新手要点（和 C 的区别）

- **C 程序员可能只做功能测试**：C 程序员写测试通常验证"结果对不对"——但并发测试要验证"在高并发下对不对"。这是思维方式的转变。
- **"随机让步"是并发测试的技巧**：C 程序员可能觉得"测试不该有随机性"——但并发 bug 正是时序随机性导致的。在测试中主动引入随机延迟，能覆盖更多交错。
- **压力测试要长时间跑**：C 程序员可能跑 1 秒就结束——但罕见竞争可能要 1 小时才触发。并发压力测试要跑足够长（至少 60 秒，最好几小时）。
- **组合测试比单一操作测试更重要**：C 程序员可能分别测 push 和 pop——但竞争可能发生在 push 和 pop 同时进行时。组合测试（多操作并发）是关键。

---

## HFT 关联

- **HFT 系统的测试投入极大**：HFT 系统的并发正确性关乎金融安全——测试投入通常占开发周期的 50% 以上。压力测试、随机调度、长时间运行是标配。
- **模拟生产负载**：HFT 测试要模拟生产环境的并发模式（如行情频率、订单频率）——开发环境的负载和生产差几个数量级，测试要接近生产。
- **夜间压力测试**：HFT 系统通常每晚跑 8 小时压力测试 + TSan——在一夜的高并发压力下暴露罕见竞争。
- **随机调度 + TSan 组合**：HFT 测试在 TSan 下用随机调度（yield/sleep）——最大化覆盖交错路径，同时检测竞争。

---

## 自测题

1. 并发测试和串行测试有什么本质区别？
2. 压力测试中为什么要插入随机 `yield` 或 `sleep`？
3. 组合测试（多操作并发）为什么比单一操作测试更重要？
4. 压力测试应该跑多长时间？为什么 1 秒不够？
5. HFT 系统的测试投入为什么占开发周期 50% 以上？

---

## 参考与延伸

- 下一节：[11.5 定位技巧](05-locating.md)
- 上一节：[11.3 死锁的检测](03-deadlock.md)
- 回到：[第 11 章](README.md)
