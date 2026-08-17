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
