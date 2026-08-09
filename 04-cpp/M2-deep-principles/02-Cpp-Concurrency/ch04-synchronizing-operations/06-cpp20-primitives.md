# 4.6 C++20 新同步原语

> 第 4 章 · 上一节：[4.5 超时等待](05-timeout.md) · 下一章：[第 5 章 内存模型和原子操作](../ch05-memory-model-atomics/README.md)

## 这节讲什么

C++20 引入 `latch`、`barrier`、`semaphore`——标准化的同步原语，替代手写 mutex+cv。

---

## 四个新原语

| 原语 | 作用 |
|------|------|
| `std::latch` | 一次性计数屏障，`count_down` + `wait`，不可重置 |
| `std::barrier` | 可复用屏障，`arrive_and_wait`，适合分阶段并行 |
| `std::counting_semaphore` | 信号量，控制并发数 |
| `std::binary_semaphore` | 二值信号量（≈ mutex 但可由非所有者释放） |

```cpp
std::latch start(3);
// 3 个线程各自准备后
start.count_down();  // 计数减一
start.wait();        // 等计数归零，一起开跑
```

---

## 新手要点

- **`latch` vs `barrier`**：`latch` 一次性（不可重置），`barrier` 可复用（分阶段并行）。
- **替代手写 mutex+cv**：以前要手写 mutex + condition_variable + 计数器实现的同步，现在一行 `latch`/`barrier` 搞定。

---

## HFT 关联

- **latch 做阶段同步**：策略初始化多阶段用 `latch` 等所有 worker 就位再开闸。
- **barrier 做批量处理**：分片行情处理用 `barrier` 同步各分片完成，再聚合。

---

## 自测题

1. `latch` 和 `barrier` 有什么区别？分别适合什么场景？
2. `binary_semaphore` 和 `mutex` 有什么不同？
3. C++20 新原语替代了什么手写模式？

---

## 参考与延伸

- 下一章：[第 5 章 内存模型和原子操作](../ch05-memory-model-atomics/README.md)
- 回到：[第 4 章](README.md)
