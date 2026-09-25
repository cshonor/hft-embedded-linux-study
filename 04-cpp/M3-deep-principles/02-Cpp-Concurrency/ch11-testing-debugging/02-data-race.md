# 11.2 数据竞争的检测

> 第 11 章 · 上一节：[11.1 并发 bug 的特征](01-bug-characteristics.md) · 下一节：[11.3 死锁的检测](03-deadlock.md)

## 这节讲什么

数据竞争（data race）= 至少一个写操作 + 无同步 + 跨线程访问同一位置——是 UB（未定义行为）。本节讲 ThreadSanitizer（TSan）的使用、它能检测什么、以及如何写"可被 TSan 检测"的测试。

---

## 核心规则（代码+表格）

### 数据竞争的定义

```cpp
// 数据竞争的三要素：
// 1. 跨线程访问同一变量
// 2. 至少一个是写操作
// 3. 无同步关系（无 happens-before）

int x = 0;
std::thread t1([&]{ x = 1; });           // 写
std::thread t2([&]{ std::cout << x; });  // 读，无同步
t1.join(); t2.join();
// → 数据竞争！UB
```

### 不是数据竞争的情况

```cpp
// 1. 有同步（join 建立 happens-before）
std::thread t1([&]{ x = 1; });
t1.join();  // join 同步：t1 的写 happens-before 主线程
std::cout << x;  // 读在 join 之后 → 无竞争

// 2. 用 atomic
std::atomic<int> x{0};
std::thread t1([&]{ x.store(1); });
std::thread t2([&]{ std::cout << x.load(); });
// atomic 操作有内存序 → 无竞争（但可能读到 0 或 1，取决于时序）

// 3. 都只读
std::thread t1([&]{ std::cout << x; });
std::thread t2([&]{ std::cout << x; });
// 都只读 → 无竞争
```

### ThreadSanitizer (TSan) 使用

```bash
# 编译时加 -fsanitize=thread
g++ -fsanitize=thread -g -O1 race.cpp -o race
./race
# 如果有数据竞争，TSan 会输出详细报告：
# WARNING: ThreadSanitizer: data race
# Write of size 4 by thread T1:
#   #0 foo() race.cpp:5
# Previous read of size 4 by thread T2:
#   #0 bar() race.cpp:10
```

### TSan 的代价和限制

| 维度 | 说明 |
|------|------|
| 性能 | 10-20 倍减速 |
| 内存 | 5-10 倍 |
| 误报 | 极少（TSan 很精确） |
| 漏报 | 有（只检测实际发生的竞争，未触发的路径检测不到） |
| 原子操作 | 支持（检测内存序错误） |
| 支持 | C/C++，Linux/macOS（Windows 支持有限） |

### 写"可被 TSan 检测"的测试

```cpp
// 反例：测试太简单，不触发竞争路径
TEST(StackTest, PushPop) {
    threadsafe_stack<int> s;
    s.push(1);
    int v;
    s.pop(v);
    EXPECT_EQ(v, 1);
}
// TSan 不会报错——因为没有并发访问

// 正解：多线程压测，暴露竞争路径
TEST(StackTest, ConcurrentPushPop) {
    threadsafe_stack<int> s;
    std::vector<std::thread> threads;
    std::atomic<int> sum{0};
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]{
            for (int i = 0; i < 10000; ++i) {
                s.push(t * 10000 + i);
                int v;
                if (s.pop(v)) sum.fetch_add(v);
            }
        });
    }
    for (auto& th : threads) th.join();
    // TSan 会检测 push/pop 实现中是否有竞争
}
```

---

## 新手要点（和 C 的区别）

- **C 程序员可能不知道 TSan**：C 的 `gcc -fsanitize=thread` 同样支持 TSan——但 C 程序员可能没用过。TSan 是并发调试的最强工具，C/C++ 通用。
- **"数据竞争 = UB"是关键认知**：C 程序员可能觉得"读到一个旧值而已，不会崩"——但 C/C++ 标准规定数据竞争是 UB，编译器可以假设不发生竞争并据此优化（如把读移出循环），导致不可预测的行为。
- **TSan 不是调试器**：C 程序员可能习惯用 gdb 调试——但并发 bug 在 gdb 下往往不复现（断点改变时序）。TSan 是运行时检测工具，在正常运行中报告竞争——这是并发调试的正确方式。
- **测试要"有并发"才能检测**：C 程序员可能写单元测试只测单线程功能——但 TSan 只检测实际发生的竞争。并发测试要多线程压测，暴露竞争路径。

---

## HFT 关联

- **HFT CI 流水线必须跑 TSan**：每次代码提交都在 TSan 下跑并发测试——10-20 倍减速但 CI 可以容忍（非实时）。生产二进制不加 TSan。
- **HFT 无锁结构尤其需要 TSan**：无锁结构的内存序错误极难发现——TSan 能检测 acquire/release 配对错误、错误的 relaxed 使用。
- **压力测试 + TSan**：HFT 系统在 TSan 下做压力测试（模拟生产负载的 10 倍并发）——暴露在正常负载下不触发的竞争。
- **Windows 支持有限**：TSan 在 Windows 上支持有限——HFT 系统如果在 Windows 开发，可能要在 Linux WSL 下跑 TSan 测试。

---

## 自测题

1. 数据竞争的三要素是什么？
2. 为什么 `t1.join()` 之后的读不算数据竞争？
3. TSan 的性能代价是多少？为什么 HFT 生产环境不用但 CI 要用？
4. 为什么单元测试要多线程压测才能被 TSan 检测到竞争？
5. 为什么说"数据竞争是 UB"而不仅仅是"读到旧值"？

<details>
<summary>参考答案</summary>

1. 两个或更多线程并发访问同一 memory location、其中至少一个是写，并且这些冲突访问不是原子操作且不存在 happens-before 顺序。
   三项同时成立才构成 C++ 定义的数据竞争，并直接导致未定义行为。
2. 被 join 线程中的所有操作 synchronize-with `join()` 的成功返回，因此 happens-before 返回后的读取。
   前提是读确实发生在 join 返回后；若还有其他未同步写线程，仍可能存在竞争。
3. TSan 会插桩内存访问并维护同步与 happens-before 元数据，常见代价是数倍到十余倍时间及显著额外内存，具体依程序而异。
   这不适合生产 HFT 的延迟与容量要求，但 CI 可接受开销以持续发现真实执行路径上的竞争。
4. TSan 是动态检测器，只能报告运行中实际观察到的冲突访问。
   多线程压力、改变调度和重复执行可扩大危险交错的覆盖面，单线程或短测试可能根本不触发冲突。
5. 编译器可基于“程序无数据竞争”的前提重排、合并或删除访问，CPU 也可能呈现非直觉可见顺序。
   因此结果不受限于旧值或丢失更新，标准不对整个程序行为作保证。

</details>

---

## 参考与延伸

- 下一节：[11.3 死锁的检测](03-deadlock.md)
- 上一节：[11.1 并发 bug 的特征](01-bug-characteristics.md)
- 回到：[第 11 章](README.md)
