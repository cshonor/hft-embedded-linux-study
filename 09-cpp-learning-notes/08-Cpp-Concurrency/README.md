# 《C++ 并发编程实战（第 2 版）》章节索引

> 正文 11 章 + 4 附录，循序渐进搭建 C++ 并发完整知识体系。路径均为 ASCII。

## 正文

| 章 | 目录 | 主题 |
|----|------|------|
| 1 | [你好，C++ 并发世界](./ch01-hello-concurrency/) | 并发/并行概念，C++11 并发能力入门 |
| 2 | [线程管控](./ch02-managing-threads/) | `std::thread` 创建、等待、分离、异常 |
| 3 | [在线程间共享数据](./ch03-sharing-data/) | 互斥锁、`lock_guard`、`unique_lock`、死锁 |
| 4 | [并发操作的同步](./ch04-synchronizing-operations/) | 条件变量、future/promise、async |
| 5 | [C++ 内存模型和原子操作](./ch05-memory-model-atomics/) | 内存序、`std::atomic`、无锁底层 |
| 6 | [设计基于锁的并发数据结构](./ch06-lock-based-containers/) | 线程安全栈、队列、哈希表 |
| 7 | [设计无锁数据结构](./ch07-lock-free-containers/) | CAS、无锁容器、内存回收 |
| 8 | [设计并发代码](./ch08-designing-concurrent-code/) | 任务划分、减少共享、通信方案 |
| 9 | [高级线程管理](./ch09-advanced-thread-management/) | 线程池、`thread_local`、中断 |
| 10 | [并行算法函数](./ch10-parallel-algorithms/) | C++17 并行 STL、执行策略（详见 [09 ch22](../09-C++17-The-Complete-Guide/ch22-parallel-stl/)） |
| 11 | [多线程应用的测试和除错](./ch11-testing-debugging/) | 数据竞争、死锁定位与测试 |

## 附录

| 附录 | 目录 | 说明 |
|------|------|------|
| A | [附录 A](./appendix-a-cpp11-primer/) | C++11 精要：右值引用、lambda、智能指针等 |
| B | [附录 B](./appendix-b-library-comparison/) | 各并发程序库简要对比 |
| C | [附录 C](./appendix-c-atm-example/) | 消息传递与 ATM 综合实战 |
| D | [附录 D](./appendix-d-thread-library-ref/) | C++11 线程库参考名录 |

## 学习路线

1. **零基础**：附录 A → 第 1–4 章（基础线程、锁、同步）
2. **进阶底层**：第 5 章（内存模型 + 原子，核心难点）
3. **数据结构实战**：第 6、7 章（有锁 → 无锁容器）
4. **工程化开发**：第 8、9、10 章（并发设计、线程池、并行 STL）
5. **线上稳定保障**：第 11 章（测试排错）
6. **拓展查阅**：附录 B/C/D（库对比、完整案例、API 速查）

## 学习进度

- [ ] 第 1 章
- [ ] 第 2 章
- [ ] 第 3 章
- [ ] 第 4 章
- [ ] 第 5 章
- [ ] 第 6 章
- [ ] 第 7 章
- [ ] 第 8 章
- [ ] 第 9 章
- [ ] 第 10 章
- [ ] 第 11 章
- [ ] 附录 A
- [ ] 附录 B
- [ ] 附录 C
- [ ] 附录 D
