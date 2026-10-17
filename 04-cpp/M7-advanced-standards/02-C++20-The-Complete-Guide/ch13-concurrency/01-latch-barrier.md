# std::latch / std::barrier

## latch：一次性计数器

```cpp
#include <latch>

// latch：倒计时计数器，到 0 后释放所有等待者
std::latch work_done(3);  // 等 3 个任务完成

// 工作线程
auto worker = [&]() {
    do_work();
    work_done.count_down();  // 计数减 1
};

// 主线程等待
work_done.wait();  // 阻塞直到计数归 0
// 或：
work_done.arrive_and_wait();  // 等价 count_down(1) + wait

// latch 是一次性的——归 0 后不能重用
```

## barrier：可重置的屏障

```cpp
#include <barrier>

// barrier：N 个线程到达后全部释放，并可执行完成回调
std::barrier sync_point(4, []() noexcept {
    // 每轮结束时执行（不用 catch 异常）
    std::cout << "Phase done\n";
});

// 4 个线程
auto worker = [&]() {
    for (int phase = 0; phase < 10; ++phase) {
        do_phase_work(phase);
        sync_point.arrive_and_wait();  // 到达并等待其他线程
        // 所有线程都到达后，执行回调，然后全部继续
    }
};
```

## latch vs barrier

| 特性 | latch | barrier |
|------|-------|---------|
| 可重用 | 一次性 | 可重置（每轮自动重置） |
| 计数 | 只减不增 | 每轮重置为 N |
| 回调 | 无 | 有完成回调 |
| 等待 | `wait()` | `arrive_and_wait()` |
| 适用 | 一次性同步 | 多轮同步 |

## HFT 应用

```cpp
// latch：等待多个策略初始化完成
std::latch init_done(num_strategies);
for (auto& strat : strategies) {
    pool.submit([&]() {
        strat.init();
        init_done.count_down();
    });
}
init_done.wait();  // 所有策略初始化完才开始交易

// barrier：多阶段批处理同步
std::barrier phase_sync(num_workers);
for (int phase = 0; phase < num_phases; ++phase) {
    // 每个线程处理自己的数据块
    process_chunk(phase);
    phase_sync.arrive_and_wait();  // 同步所有线程
    // 所有线程都完成当前阶段后继续
}
```

## 自测题

1. `latch` 和 `barrier` 的区别？
2. `latch` 能重用吗？`barrier` 呢？
3. `barrier` 的完成回调做什么？什么时候执行？
4. `arrive_and_wait` 和 `count_down` + `wait` 的区别？
5. HFT 中如何用 `latch` 等待多策略初始化？

<details>
<summary>参考答案</summary>

1. `std::latch`：**一次性倒计时门闩**。内部有一个只减不增的计数，`count_down()` 递减，`wait()` 阻塞直到计数到 0；到 0 后所有等待者放行，状态永久保持。
`std::barrier`：**可重复使用的屏障**。固定参与线程数（构造时给定），每一轮中线程 `arrive_and_wait()` 到达并阻塞，最后一个到达时触发完成回调，然后**全体同时放行**并自动进入下一轮。
一句话：latch 是"一次性开门"，barrier 是"每轮集合点"。
2. `latch` **不能重用**：计数到 0 后就永久打开，无法再加回去（没有"加计数"的接口），想再用只能新建对象。
`barrier` **可以重用**：每轮结束（所有线程到达、回调执行完）后自动重置到初始状态，可直接用于下一轮循环，这正是它在"多阶段批处理"里反复 `arrive_and_wait()` 的原因。
3. 完成回调（completion function）是构造 `barrier` 时的第二个参数，用于**每轮**结束时做一次性的阶段汇总或共享状态重置（如交换双缓冲、清空本轮累积的统计）。
执行时机：在**该轮最后一个到达的线程**上、该线程阻塞期间被调用；回调执行完之后，所有线程才被同时放行。
约束：回调必须 **`noexcept`**（抛出会 `std::terminate`），且不能在回调里调用同一个 barrier 的 `arrive_and_wait()`（会死锁）。
4. `arrive_and_wait()` 是**一步到位**："我到达了" + "阻塞等待其他人"，到达与等待是同一个操作，调用后线程立即停在那里。
`count_down()` + `wait()` 是**分开的两步**：`count_down()` 只递减计数、**不阻塞**，线程可以接着干点别的（比如先把自己那部分结果写好），之后再调用 `wait()` 阻塞等待。
两者语义等价（都是"到达"与"等待"），区别只在于到达与等待之间能否插入工作；latch 提供这两种写法，barrier 主要是 `arrive_and_wait()`（另有 `arrive_and_drop()` 表示退出参与）。
5. ```cpp
std::latch init_done(num_strategies);
for (auto& strat : strategies) {
    pool.submit([&]() {
        strat.init();            // 各策略并行初始化
        init_done.count_down();  // 完成一个
    });
}
init_done.wait();                // 全部就绪后才开始交易
```
要点：计数初始为策略数；每个任务结束时 `count_down()`；主线程 `wait()` 一次性等到全部完成。因为 latch 是一次性的，它天然适合"启动/初始化的一次性同步"——反复出现的阶段同步则改用 `barrier`。

</details>
