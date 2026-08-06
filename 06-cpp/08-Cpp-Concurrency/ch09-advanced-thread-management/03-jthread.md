# 9.3 std::jthread 与协作式中断

> 第 9 章 · 上一节：[9.2 Work-Stealing 调度](02-work-stealing.md) · 下一节：[9.4 stop_callback](04-stop-callback.md)

## 这节讲什么

C++20 引入 `std::jthread`（joining thread）——析构时自动 join，并内置 `stop_token` 实现协作式中断。本节讲 `jthread` vs `thread` 的区别、`stop_token` 的使用模式、以及为什么"协作式"中断比"强制 kill"更安全。

---

## 核心规则（代码+表格）

### `jthread` vs `thread`

```cpp
// C++11 std::thread：析构时未 join → terminate
{
    std::thread t([]{ long_work(); });
    // 忘记 t.join() → 析构 → std::terminate！
}

// C++20 std::jthread：析构时自动 join
{
    std::jthread t([]{ long_work(); });
    // 析构自动 join，安全
}

// jthread 还支持请求中断
{
    std::jthread t([](std::stop_token st){
        while (!st.stop_requested()) {
            do_chunk();
        }
    });
    // ...
    t.request_stop();  // 请求中断
    // 析构时自动 join，等线程退出循环
}
```

### `stop_token` 协作式中断

```cpp
// 线程函数接收 stop_token，定期检查
void worker(std::stop_token st) {
    while (!st.stop_requested()) {
        auto chunk = get_next_chunk();
        if (chunk.empty()) break;
        process(chunk);
    }
    cleanup();  // 中断后仍能清理
}

// 外部请求中断
std::jthread t(worker);
// ... 某个条件触发 ...
t.request_stop();  // st.stop_requested() 变 true
// 线程在下一次检查时退出循环

// 也可以析构时自动 request_stop + join
{
    std::jthread t(worker);
    // 作用域结束 → 析构 → request_stop() + join()
}
```

### 协作式 vs 强制式中断

| 方式 | 机制 | 安全性 | C++ 支持 |
|------|------|--------|---------|
| 协作式 | 线程自己检查标志退出 | 高（线程控制清理） | `stop_token`（C++20） |
| 强制 kill | 外部终止线程 | 低（资源泄漏、锁未释放） | **不支持**（故意不提供） |
| `pthread_cancel` | 异步取消 | 低 | C 有，C++ 不推荐 |

### `stop_source` / `stop_token` / `stop_callback` 三件套

```cpp
std::jthread t([](std::stop_token st){
    // 注册回调：stop 请求时自动调用
    std::stop_callback cb(st, []{
        std::cout << "stop requested!\n";
    });
    while (!st.stop_requested()) { work(); }
});

// t 内部持有 stop_source
// request_stop() → stop_source.request_stop() → st.stop_requested() = true
// stop_callback 在 request_stop() 时同步调用
```

| 组件 | 作用 |
|------|------|
| `stop_source` | 拥有停止状态，可调用 `request_stop()` |
| `stop_token` | 共享停止状态，可查询 `stop_requested()` |
| `stop_callback` | 注册回调，stop 请求时调用 |

---

## 新手要点（和 C 的区别）

- **C 的 `pthread_cancel` 是强制取消**：C 程序员可能习惯 `pthread_cancel`——但它极其危险：线程可能在持锁时被取消，导致锁永不释放。C++ 故意不提供类似的强制取消，而是用协作式 `stop_token`。
- **"协作式"的含义**：外部不能强制杀线程，只能"请求"线程停止——线程自己决定何时、如何退出。这让线程能在退出前释放锁、关闭文件、清理资源。C 程序员要改变"想杀就杀"的思维。
- **`jthread` 析构自动 join 是 C 程序员的好消息**：C 里 `pthread_join` 忘记调用 → 线程变成僵尸或资源泄漏。C++20 的 `jthread` 让这个错误不可能发生——RAII 自动处理。
- **`stop_token` 的检查频率**：协作式中断要求线程"定期检查" `stop_requested()`——如果线程在一个长操作中不检查，中断就无效。C 程序员可能不习惯这个"必须主动检查"的模式。

---

## HFT 关联

- **HFT 系统的优雅退出**：HFT 系统关机时不能直接杀线程——可能有未完成的订单、未刷盘的日志。`stop_token` 让每个线程在安全点退出：完成当前 tick 处理 → 刷盘 → 退出。
- **`jthread` 替代 `thread + guard`**：HFT 系统中如果用 `std::thread`，通常要配 `thread_guard`（RAII join）。`jthread` 内置了这个功能——更简洁、更不易出错。
- **`stop_callback` 用于通知**：HFT 系统关机时，`stop_callback` 可以通知其他组件（如"策略线程正在退出，别再提交任务"）——比手动标志 + 轮询更优雅。
- **热路径不检查 `stop_requested()`**：HFT 热路径（行情处理循环）每纳秒都珍贵——在循环中检查 `stop_requested()` 有分支预测开销。通常在批次之间（如每 1000 个 tick）检查一次。

---

## 自测题

1. `std::jthread` 和 `std::thread` 有什么区别？析构行为有何不同？
2. 什么是"协作式中断"？为什么比"强制 kill"更安全？
3. `stop_source`、`stop_token`、`stop_callback` 各自的作用是什么？
4. 为什么 C++ 不提供类似 `pthread_cancel` 的强制取消？
5. HFT 系统关机时如何用 `stop_token` 实现优雅退出？

---

## 参考与延伸

- 下一节：[9.4 stop_callback](04-stop-callback.md)
- 上一节：[9.2 Work-Stealing 调度](02-work-stealing.md)
- 回到：[第 9 章](README.md)
