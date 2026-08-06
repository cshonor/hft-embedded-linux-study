# 8.6 负载均衡

> 第 8 章 · 上一节：[8.5 可扩展性法则](05-scalability.md) · 下一章：[9.1 线程池基础](../ch09-advanced-thread-management/01-thread-pool.md)

## 这节讲什么

多线程系统如果任务分配不均——有的线程忙死、有的闲死——并行效率大打折扣。本节讲静态分配 vs 动态分配、work-stealing（工作窃取）调度、以及 HFT 中为什么通常用静态绑核而非动态负载均衡。

---

## 核心规则（代码+表格）

### 静态分配 vs 动态分配

```cpp
// 静态分配：启动时固定，运行中不变
void static_assign() {
    // 线程0 处理标的 0-999
    // 线程1 处理标的 1000-1999
    // ...
    for (int i = 0; i < num_threads; ++i)
        threads.emplace_back(worker, i * 1000, (i+1) * 1000);
}

// 动态分配：共享任务队列，谁空谁取
std::mutex task_mutex;
std::queue<Task> task_queue;
void dynamic_worker() {
    for (;;) {
        Task t;
        {
            std::lock_guard<std::mutex> lk(task_mutex);
            if (task_queue.empty()) return;
            t = task_queue.front();
            task_queue.pop();
        }
        process(t);
    }
}
```

| 方式 | 优点 | 缺点 | 适用 |
|------|------|------|------|
| 静态分配 | 无竞争、cache 友好 | 负载不均时效率低 | 任务耗时均匀 |
| 动态分配 | 自动均衡 | 队列竞争开销 | 任务耗时差异大 |
| work-stealing | 兼顾两者 | 实现复杂 | 通用 |

### Work-Stealing 调度

```cpp
// 每线程有自己的任务队列（双端队列）
// 自己从队尾取任务；空闲时从别人的队头"偷"任务
class work_stealing_pool {
    std::vector<std::deque<Task>> queues;  // 每线程一个
    std::vector<std::mutex> queue_mutexes;
    std::condition_variable cv;
    std::atomic<bool> stop{false};

    void worker(int my_id) {
        while (!stop) {
            Task t;
            // 1. 先从自己的队列取
            {
                std::lock_guard<std::mutex> lk(queue_mutexes[my_id]);
                if (!queues[my_id].empty()) {
                    t = std::move(queues[my_id].back());
                    queues[my_id].pop_back();
                }
            }
            // 2. 自己空了 → 偷别人的
            if (!t.valid()) {
                for (int i = 1; i < queues.size(); ++i) {
                    int victim = (my_id + i) % queues.size();
                    std::lock_guard<std::mutex> lk(queue_mutexes[victim]);
                    if (!queues[victim].empty()) {
                        t = std::move(queues[victim].front());  // 偷队头
                        queues[victim].pop_front();
                        break;
                    }
                }
            }
            if (t.valid()) process(t);
            else cv.wait(...);
        }
    }
};
```

### Work-Stealing 的关键设计

| 要点 | 说明 |
|------|------|
| 自己取队尾，偷别人队头 | 减少 CAS 竞争（LIFO 本地 + FIFO 偷） |
| 偷是最后手段 | 先消费本地，减少全局竞争 |
| 偷的代价 | 每次偷要锁 victim 的队列 |
| Java ForkJoinPool / TBB | 工业级 work-stealing 实现 |

### 负载不均的危害

```
4 线程，100 任务，每任务 1ms
  理想（均衡）：25ms
  最差（3 线程分到 1 任务，1 线程分到 97）：97ms

  → 静态分配的极端情况
  → 动态分配/work-stealing 能避免这种极端
```

---

## 新手要点（和 C 的区别）

- **C 程序员通常用静态分配**：`pthread_create` 时传参固定每个线程的任务范围。简单但不灵活——任务耗时不均时效率差。
- **work-stealing 是 C 程序员陌生的概念**：C 里几乎没有现成的 work-stealing 库。C++ 有 Intel TBB（`tbb::parallel_for` 底层就是 work-stealing）。手写 work-stealing 非常复杂，通常用现成库。
- **动态分配的队列竞争**：C 程序员可能用一个共享队列 + mutex 做动态分配——但队列本身是竞争点。work-stealing 的"每线程本地队列 + 偶尔偷"是更优的设计。
- **"先消费本地"是核心思想**：C 程序员可能习惯"所有任务放一个队列，大家抢"——但全局队列竞争激烈。work-stealing 的"本地优先"让大部分操作无竞争。

---

## HFT 关联

- **HFT 通常用静态绑核，不用动态负载均衡**：HFT 追求低延迟和确定性——动态分配的队列竞争和 work-stealing 的不确定性（不知道什么时候被偷）会引入延迟抖动。静态绑核 = 每线程固定处理特定标的 = 延迟可预测。
- **行情分发的静态分配**：按股票代码 hash 分配到各线程——线程0 处理 000xxx，线程1 处理 001xxx。每线程独立处理自己的标的，无需负载均衡。
- **work-stealing 用于盘后批处理**：盘后回测、因子计算可以用 TBB 的 work-stealing——这些场景不追求低延迟，追求高吞吐和负载均衡。
- **绑核 + CPU 亲和性**：HFT 中静态分配配合 `pthread_setaffinity_np`（或 Windows `SetThreadAffinityMask`）把线程绑在固定核上——避免 OS 调度器迁移线程导致 cache 冷启动。

---

## 自测题

1. 静态分配和动态分配各有什么优缺点？什么时候选动态分配？
2. Work-stealing 的基本原理是什么？"自己取队尾、偷别人队头"有什么好处？
3. 为什么 work-stealing 比单一共享队列的动态分配更好？
4. 为什么 HFT 系统通常用静态绑核而非动态负载均衡？
5. 什么 HFT 场景适合用 work-stealing？

---

## 参考与延伸

- 下一章：[9.1 线程池基础](../ch09-advanced-thread-management/01-thread-pool.md)
- 上一节：[8.5 可扩展性法则](05-scalability.md)
- 回到：[第 8 章](README.md)
