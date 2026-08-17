# 11.5 定位技巧

> 第 11 章 · 上一节：[11.4 并发测试策略](04-testing.md) · 下一节：[11.6 常见陷阱](06-pitfalls.md)

## 这节讲什么

当并发 bug 发生时，如何定位根因？本节讲核心转储分析、硬件性能计数器（perf）、日志的艺术（不加 print 而用无锁日志）、以及"二分法"在并发调试中的应用。

---

## 核心规则（代码+表格）

### 定位工具链

| 工具 | 用途 | 侵入性 |
|------|------|--------|
| TSan | 运行时检测竞争 | 高（10-20x 减速） |
| gdb + core dump | 崩溃后分析栈 | 低（仅崩溃时） |
| perf | 硬件性能计数器 | 极低 |
| strace/ltrace | 系统调用追踪 | 中 |
| 自定义无锁日志 | 记录关键事件 | 低 |

### Core Dump 分析

```bash
# 启用 core dump
ulimit -c unlimited
# 运行程序，崩溃后生成 core 文件
./hft_system  # 崩溃 → core.12345

# gdb 分析
gdb ./hft_system core.12345
(gdb) bt              # 所有线程的回溯
(gdb) thread apply all bt  # 所有线程栈
(gdb) info threads    # 线程列表
(gdb) thread 3        # 切到线程 3
(gdb) frame 5         # 切到栈帧 5
(gdb) print variable  # 查看变量
```

### perf：硬件计数器

```bash
# 统计 cache miss、分支预测失败等
perf stat ./hft_system

# 热点分析
perf record ./hft_system
perf report

# 缓存行争用（false sharing 检测）
perf stat -e cache-misses,LLC-load-misses ./hft_system
# 如果 cache-miss 异常高，可能有 false sharing
```

perf 的优势：**几乎零侵入**——它用硬件性能计数器，不改变时序，不会导致 Heisenbug。

### 无锁日志：不改变时序

```cpp
// 反例：mutex 日志改变时序
void bad_log(const std::string& msg) {
    std::lock_guard<std::mutex> lk(m);
    std::cout << msg << "\n";  // 锁 + I/O → 改变时序
}

// 正解：每线程无锁环形缓冲
thread_local char log_buf[4096];
thread_local size_t log_pos = 0;

void lock_free_log(const char* msg, size_t len) {
    if (log_pos + len < 4096) {
        memcpy(log_buf + log_pos, msg, len);
        log_pos += len;
    }
}
// 写入 thread_local 缓冲，无锁、无 I/O → 不改变时序
// 崩溃后从 core dump 提取各线程的 log_buf
```

### 二分法定位

```cpp
// 当不确定哪段代码引入竞争时，用二分法
// 1. 注释掉一半功能，看竞争是否消失
// 2. 如果消失 → 竞争在注释掉的部分
// 3. 如果不消失 → 竞争在保留的部分
// 4. 继续二分

// 实操：用 #ifdef 控制
#ifdef DEBUG_HALF
    // strategy_a();
#endif
    strategy_b();
    strategy_c();
// 跑测试，看竞争是否还在
```

---

## 新手要点（和 C 的区别）

- **C 程序员习惯 gdb 调试**：gdb 在并发调试中仍有用（core dump 分析），但断点调试会改变时序导致 Heisenbug。C 程序员要改掉"断点单步"的习惯，改用 core dump 事后分析。
- **perf 是 C 程序员可能不熟悉的工具**：C 程序员可能用 `strace`/`gprof`——但 `perf` 更强大（硬件计数器、零侵入）。HFT 调试必备。
- **无锁日志是新概念**：C 程序员习惯 `printf` 调试——但在并发中，`printf` 的锁和 I/O 改变时序。C++ 的 `thread_local` 缓冲 + 事后提取是更好的方案。
- **二分法是通用技巧**：C 程序员应该熟悉二分法调试——但在并发中更重要，因为并发 bug 难以直接观察。注释代码 + 看竞争是否消失是有效手段。

---

## HFT 关联

- **HFT 调试用 perf 而非 print**：HFT 系统的纳秒级时序对 print 极其敏感——一个 `printf` 就可能改变竞争结果。perf 的零侵入特性是 HFT 调试的首选。
- **core dump 是 HFT 崩溃分析的标准**：HFT 系统必须启用 core dump——崩溃后从 core 提取所有线程栈，分析死锁或崩溃原因。
- **无锁日志 + core dump**：HFT 系统的日志写入 `thread_local` 环形缓冲，崩溃后从 core dump 提取——这是 HFT 的标准调试流程。
- **false sharing 检测**：HFT 系统如果性能不及预期，用 `perf stat -e cache-misses` 检测——异常高的 cache miss 可能是 false sharing（未 `alignas(64)`）。

---

## 自测题

1. 为什么在并发调试中用 gdb 断点单步会导致 Heisenbug？
2. perf 相比 print/printf 有什么优势？为什么适合 HFT？
3. 无锁日志（`thread_local` 缓冲）如何避免改变时序？
4. core dump 分析能做什么？如何查看所有线程的栈？
5. 如何用二分法定位并发竞争？

---

## 参考与延伸

- 下一节：[11.6 常见陷阱](06-pitfalls.md)
- 上一节：[11.4 并发测试策略](04-testing.md)
- 回到：[第 11 章](README.md)
