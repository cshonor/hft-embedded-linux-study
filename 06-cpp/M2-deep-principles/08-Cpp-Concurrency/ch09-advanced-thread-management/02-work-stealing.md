# 9.2 Work-Stealing 调度

> 第 9 章 · 上一节：[9.1 线程池基础](01-thread-pool.md) · 下一节：[9.3 jthread 与协作式中断](03-jthread.md)

## 这节讲什么

基础线程池用单一任务队列——提交任务时所有线程争抢同一把锁。Work-stealing 让每个线程有自己的本地队列，空闲时从别人队列"偷"任务。本节讲 work-stealing 线程池的设计、偷任务的方向、以及它在并行递归（fork/join）中的优势。

---

## 核心规则（代码+表格）

### Work-Stealing 线程池结构

```cpp
class work_stealing_pool {
    // 每线程一个本地双端队列
    std::vector<std::deque<std::function<void()>>> local_queues;
    std::vector<std::mutex> queue_mutexes;
    std::condition_variable cv;
    std::mutex cv_mutex;
    std::atomic<bool> stop{false};
    std::atomic<unsigned> active_count{0};
    unsigned num_threads;

    void worker(unsigned my_id) {
        while (!stop) {
            std::function<void()> task;
            // 1. 先从自己的本地队列取（LIFO，从尾部）
            {
                std::lock_guard<std::mutex> lk(queue_mutexes[my_id]);
                if (!local_queues[my_id].empty()) {
                    task = std::move(local_queues[my_id].back());
                    local_queues[my_id].pop_back();
                }
            }
            // 2. 本地空了 → 偷别人的（FIFO，从头部）
            if (!task) {
                for (unsigned i = 1; i < num_threads; ++i) {
                    unsigned victim = (my_id + i) % num_threads;
                    std::lock_guard<std::mutex> lk(queue_mutexes[victim]);
                    if (!local_queues[victim].empty()) {
                        task = std::move(local_queues[victim].front());
                        local_queues[victim].pop_front();
                        break;
                    }
                }
            }
            // 3. 执行
            if (task) {
                active_count.fetch_add(1);
                task();
                active_count.fetch_sub(1);
            } else {
                // 全空 → 等待
                std::unique_lock<std::mutex> lk(cv_mutex);
                cv.wait_for(lk, std::chrono::milliseconds(1));
            }
        }
    }

public:
    void submit(unsigned thread_id, std::function<void()> task) {
        // 提交到指定线程的本地队列
        std::lock_guard<std::mutex> lk(queue_mutexes[thread_id]);
        local_queues[thread_id].push_back(std::move(task));
        cv.notify_one();
    }
};
```

### LIFO 本地 + FIFO 偷的设计

| 操作 | 方向 | 原因 |
|------|------|------|
| 自己取任务 | 队尾（LIFO） | 刚提交的任务 cache 最热 |
| 偷别人任务 | 队头（FIFO） | 队头是最老的任务，本地线程不太需要它 |

这个设计让"偷"和"本地取"操作不同端，减少队列锁竞争。

### 并行递归（fork/join）的优势

```cpp
// 并行快速排序：work-stealing 的经典场景
template <typename T>
void parallel_sort(work_stealing_pool& pool, std::vector<T>& v) {
    if (v.size() < 10000) { std::sort(v.begin(), v.end()); return; }
    
    auto pivot = v[v.size()/2];
    std::vector<T> left, right;
    for (auto& x : v) (x < pivot ? left : right).push_back(x);

    // fork：提交左半部分到池
    auto fut = pool.submit_async([&]{ parallel_sort(pool, left); });
    // 自己处理右半部分
    parallel_sort(pool, right);
    // join：等左半完成
    fut.wait();

    v = std::move(left);
    v.push_back(pivot);
    v.insert(v.end(), right.begin(), right.end());
}
// work-stealing 让深度递归的任务自动均衡到空闲线程
```

### Work-Stealing vs 单队列对比

| 维度 | 单队列线程池 | Work-Stealing |
|------|-------------|---------------|
| 提交竞争 | 所有线程争一个锁 | 只争本地锁 |
| 负载均衡 | 天然均衡 | 靠偷任务均衡 |
| 递归任务 | 可能栈溢出（等子任务） | 本地处理+偷，高效 |
| 实现复杂度 | 低 | 高 |
| cache 局部性 | 差（随机取） | 好（LIFO 取热数据） |

---

## 新手要点（和 C 的区别）

- **C 里几乎没有现成的 work-stealing 库**：C 程序员要用 work-stealing 通常得自己实现（极复杂）或用 Intel TBB 的 C 接口。C++ 有 TBB 的 C++ 接口（`tbb::parallel_for`），底层是 work-stealing。
- **"本地队列 + 偷"是 C 程序员陌生模式**：C 程序员习惯"一个队列大家抢"——简单但竞争激烈。work-stealing 的"每线程本地 + 偶尔偷"是更优设计，但实现复杂度高。
- **LIFO + FIFO 的方向设计**：C 程序员可能不理解"自己从尾部取、别人从头部偷"——这是为了减少竞争（两端操作）和保持 cache 局部性（LIFO 取热数据）。这个设计来自 Java 的 ForkJoinPool 和 Cilk。
- **并行递归是 work-stealing 的杀手级应用**：C 程序员写并行排序通常手动分块——但递归分治的深度不确定，手动分块很难均衡。work-stealing 让递归任务自动均衡——这是它最大的价值。

---

## HFT 关联

- **HFT 热路径不用 work-stealing**：work-stealing 的"偷"有不确定性（不知道什么时候被偷、偷谁的）——这对 HFT 的延迟确定性是灾难。HFT 用静态绑核 + 固定流水线。
- **盘后回测用 TBB 的 work-stealing**：回测系统的任务（按标的或按时间段划分）耗时差异大，work-stealing 自动均衡。`tbb::parallel_for` 比手写分块简洁且高效。
- **并行因子计算**：多因子模型计算可以递归分治——work-stealing 让因子计算的子任务自动分配到空闲核。
- **如果必须手写 work-stealing**：HFT 系统中如果有动态任务需求，通常用无锁双端队列（如 Chase-Lev deque）而非 mutex 保护的 `std::deque`——mutex 版在偷任务时有锁竞争，无锁版更优但极难实现正确。

---

## 自测题

1. Work-stealing 中"自己取队尾、偷别人队头"的设计有什么好处？
2. 为什么 work-stealing 比单队列线程池更适合并行递归（fork/join）？
3. Work-stealing 的"偷"什么时候发生？有什么不确定性？
4. 为什么 HFT 热路径不用 work-stealing？
5. 什么 HFT 场景适合用 work-stealing？

---

## 参考与延伸

- 下一节：[9.3 jthread 与协作式中断](03-jthread.md)
- 上一节：[9.1 线程池基础](01-thread-pool.md)
- 回到：[第 9 章](README.md)
