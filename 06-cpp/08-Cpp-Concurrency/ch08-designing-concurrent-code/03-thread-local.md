# 8.3 减少共享：线程局部化

> 第 8 章 · 上一节：[8.2 数据并行：分块处理](02-data-parallel.md) · 下一节：[8.4 异常安全](04-exception-safety.md)

## 这节讲什么

多核并行的最大敌人是**共享**——任何共享变量的访问都需要同步（锁或原子），同步带来竞争和 cache bounce。本节讲 `thread_local`、线程私有缓冲、以及"尽量不共享"的可扩展性设计原则。

---

## 核心规则（代码+表格）

### `thread_local` 基础

```cpp
// 每个线程有独立的副本，无需同步
thread_local int counter = 0;  // 每线程独立

void worker() {
    counter++;  // 无竞争，每线程操作自己的副本
}

// 实用场景：线程局部日志缓冲
thread_local std::string log_buffer;

void log(const std::string& msg) {
    log_buffer += msg + "\n";  // 无锁
    if (log_buffer.size() > 4096) {
        flush_to_file(log_buffer);  // 满了才刷盘
        log_buffer.clear();
    }
}
```

### 线程私有缓冲模式

```cpp
// 反例：共享日志缓冲 + 锁
std::mutex log_mutex;
std::string shared_log;
void bad_log(const std::string& msg) {
    std::lock_guard<std::mutex> lk(log_mutex);
    shared_log += msg + "\n";  // 每次调用都争锁
}

// 正解：每线程私有缓冲，满了批量刷盘
thread_local std::string tls_log;
std::mutex file_mutex;

void good_log(const std::string& msg) {
    tls_log += msg + "\n";   // 无锁，写线程私有内存
    if (tls_log.size() > 4096) {
        std::lock_guard<std::mutex> lk(file_mutex);
        write_to_file(tls_log);  // 只在刷盘时争锁
        tls_log.clear();
    }
}
// 锁竞争频率降低 100x（每 4096 字节争一次 vs 每条日志争一次）
```

### 共享 vs 局部性能对比

| 模式 | 同步开销 | cache 效果 | 扩展性 |
|------|----------|-----------|--------|
| 全局共享 + 锁 | 每次操作争锁 | cache line bounce | 差（核数越多越慢） |
| 全局共享 + 原子 | 每次操作 CAS | cache line bounce | 差 |
| `thread_local` | 无 | 每线程独占 cache | 优（线性扩展） |
| SPSC 队列传递 | 队列两端 | 各占一端 cache | 优 |

### `thread_local` 的生命周期陷阱

```cpp
// 陷阱：thread_local 对象在线程退出时析构
thread_local std::ofstream log_file("app.log");  // 每线程打开同一个文件？
// 问题：多个线程的 ofstream 析构会关闭同一文件 → 可能竞争

// 正解：用指针 + 手动管理
thread_local std::ofstream* log_file = nullptr;
void init_thread_log() {
    log_file = new std::ofstream("thread_" + std::to_string(get_tid()) + ".log");
}
// 析构要在线程退出前手动处理，或用 atexit
```

---

## 新手要点（和 C 的区别）

- **C 里的"线程局部"是 `__thread`（GCC）或 `__declspec(thread)`（MSVC）**：C11 有 `_Thread_local`。C++11 的 `thread_local` 统一了。语义相同——每线程独立副本。
- **C 程序员容易过度共享**：C 里全局变量很常见，多线程下全局变量 = 共享 = 同步。C++ 的 `thread_local` 是减少共享的利器——C 程序员要改掉"全局变量到处用"的习惯。
- **`thread_local` 的析构顺序**：C 的 `__thread` 不调用析构（C 没有析构函数）。C++ 的 `thread_local` 对象在线程退出时析构——这可能导致析构顺序问题（如全局 `thread_local` 指针指向已析构的全局对象）。C 程序员转型时要注意。
- **"线程私有缓冲 + 批量刷盘"模式**：C 程序员可能习惯每次 `printf` 就加锁——这在多线程下是灾难。C++ 的 `thread_local` 缓冲让这个模式很自然。

---

## HFT 关联

- **`thread_local` 是 HFT 日志的标准方案**：HFT 系统日志量巨大（每笔订单、每个 tick），共享日志缓冲的锁竞争不可接受。每线程 `thread_local` 缓冲 + 批量刷盘是标配。
- **HFT 的极致是不共享**：HFT 系统设计的第一原则是"线程间不共享可变状态"——每线程有自己的行情副本、策略状态、订单队列，靠 SPSC 队列传递消息。`thread_local` 是实现这一原则的工具之一。
- **`thread_local` 的初始化开销**：`thread_local` 非平凡对象在首次访问时构造——如果对象重（如大数组），首次访问有延迟。HFT 线程启动时应预热（显式访问一次），避免运行时首次构造。
- **NUMA 感知**：`thread_local` 数据天然跟随线程的 NUMA 节点（因为线程绑核）。这是 `thread_local` 在 HFT 中的额外优势——全局共享变量可能在远端 NUMA 节点，访问延迟翻倍。

---

## 自测题

1. `thread_local` 变量和全局变量有什么区别？为什么 `thread_local` 不需要同步？
2. "线程私有缓冲 + 批量刷盘"模式比"共享缓冲 + 每次锁"好在哪里？
3. `thread_local` 对象的析构什么时候发生？有什么陷阱？
4. 为什么 C 程序员"全局变量到处用"的习惯在多线程下是问题？
5. HFT 系统为什么把"不共享可变状态"作为第一设计原则？

---

## 参考与延伸

- 下一节：[8.4 异常安全](04-exception-safety.md)
- 上一节：[8.2 数据并行：分块处理](02-data-parallel.md)
- 回到：[第 8 章](README.md)
