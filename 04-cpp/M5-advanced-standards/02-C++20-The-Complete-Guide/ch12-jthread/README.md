# 第 12 章 std::jthread 与停止令牌

**std::jthread and Stop Tokens**

## 本章讲什么

C++20 的 `std::jthread`（joining thread）是 `std::thread` 的升级版——析构自动 join（不会 terminate）+ 内置协作式中断（`stop_token`）。（并发基础见 [08 第 9 章](../../../M4-deep-principles/02-Cpp-Concurrency/ch09-advanced-thread-management/)。）

## 要点

### `jthread` vs `thread`

| 特性 | `std::thread` | `std::jthread` |
|------|---------------|----------------|
| 析构未 join/detach | `std::terminate` | 自动 join |
| 中断机制 | 无 | `request_stop()` + `stop_token` |
| 安全性 | 易错（忘 join 就崩） | 默认安全 |
| 传 stop_token | 不支持 | 自动传入 |

### 基本用法

```cpp
#include <thread>
#include <stop_token>

// jthread 自动传 stop_token
std::jthread worker([](std::stop_token st){
    while (!st.stop_requested()) {
        do_work();
    }
    cleanup();  // 优雅退出
});

// 请求停止
worker.request_stop();

// 析构时自动 join（即使没调 request_stop）
// jthread 析构 → request_stop() + join()
```

### `stop_token` 的协作式中断

```cpp
// 线程函数检查 stop_token
void run(std::stop_token st) {
    while (!st.stop_requested()) {
        process_next();
        // 或在长任务中定期检查
        for (int i = 0; i < 1000 && !st.stop_requested(); ++i) {
            process_chunk(i);
        }
    }
}
```

**协作式**：请求方设标志，线程自己检查决定退出点。不能强制杀线程——强制杀会导致资源泄漏、锁未释放。

### `stop_callback`：停止时回调

```cpp
std::jthread t([](std::stop_token st){
    while (!st.stop_requested()) { work(); }
});

// 注册停止回调
std::stop_callback cb(t.get_stop_token(), []{
    std::cout << "thread stopping\n";
});

t.request_stop();   // 触发回调
```

### `stop_source` / `stop_token` / `stop_callback` 三件套

| 类型 | 角色 |
|------|------|
| `stop_source` | 拥有停止状态，可 `request_stop()` |
| `stop_token` | 观察停止状态，可 `stop_requested()` |
| `stop_callback` | 注册停止时执行的回调 |

`jthread` 内部持有一个 `stop_source`，线程函数收 `stop_token`。也可独立用 `stop_source` 做自定义中断逻辑。

### 策略热切换示例

```cpp
class StrategyRunner {
    std::jthread worker;
public:
    void start() {
        worker = std::jthread([this](std::stop_token st){
            while (!st.stop_requested()) {
                auto tick = queue.pop();
                strategy.on_tick(tick);
            }
            strategy.on_stop();  // 优雅退出
        });
    }
    void switch_strategy() {
        worker.request_stop();   // 请求旧策略停止
        worker.join();           // 等待退出
        // 旧策略已 cleanup
        start();                 // 启动新策略
    }
};
```

## HFT 关联

- **管理线程用 jthread**：监控、日志、策略热切换等管理线程用 `jthread`，析构自动 join 不崩溃。
- **策略热切换**：`request_stop()` + `join()` 让旧策略优雅退出（排空队列、保存状态），再启动新策略。
- **`stop_token` 定期检查**：热路径线程在循环中检查 `stop_requested()`，但不要每条 tick 都检查（原子读有开销）——每 N 条检查一次。
- **热路径仍用 thread 绑核**：`jthread` 析构 join 有阻塞，纳秒级热路径仍用裸 `thread` + 手动管理。
- **`stop_callback` 清理资源**：策略停止时自动触发资源清理（关连接、存状态）。
- **协作式不阻塞**：`request_stop()` 立即返回，不等线程退出——`join()` 才等。HFT 可先 `request_stop()` 让旧策略后台退，同时准备新策略。

## 自测题

1. `jthread` 相比 `thread` 的两个核心改进？
2. `stop_token` 的协作式中断和强制杀线程的区别？为什么不能强制杀？
3. `stop_source`/`stop_token`/`stop_callback` 的角色分别是什么？
4. HFT 策略热切换如何用 `jthread` + `stop_token` 实现？
5. 为什么 HFT 热路径仍用裸 `thread` 而非 `jthread`？

## 代码自测

### Q1: jthread 自动 join
```cpp
// C++11: std::thread，忘记 join → terminate
std::thread t([] { /* work */ });
t.join();  // 必须显式 join

// C++20: std::jthread，析构自动 join + 支持取消
std::jthread jt([] { while (true) { /* work */ } });
// jt 析构时自动 join（不会 terminate）
// 支持协作取消：stop_token

std::jthread jt2([](std::stop_token st) {
    while (!st.stop_requested()) {
        // work
    }
});
jt2.request_stop();  // 请求停止
```
> jthread 比 thread 多了什么？stop_token 机制如何工作？

<details>
<summary>答案与复习指引</summary>

**jthread vs thread**：
1. **析构自动 join**：jthread 析构时如果仍 joinable，自动 join（而非 terminate）。RAII 安全。
2. **协作取消**：支持 `stop_token`，线程可以检查 `stop_requested()` 并优雅退出。

**stop_token 机制**：
- `request_stop()` 设置停止标志（原子操作，线程安全）
- 线程内 `st.stop_requested()` 检查标志
- 是**协作式**取消——线程自己决定何时/如何停止，不是强制 kill

**HFT**：jthread 适合后台任务（日志线程、监控线程）。但热路径仍用固定线程池 + 绑核，不用 jthread（线程创建/销毁开销）。`stop_token` 机制可用于优雅关机。

**复习：** → [jthread](./README.md)
</details>
