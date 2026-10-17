# 停止令牌深入：stop_source / stop_token / stop_callback

## 三件套的分工

C++20 的停止机制把"请求停止"和"检测停止"拆成两个对象，中间可以加回调：

```
stop_source ──request_stop()──▶ 共享停止状态 ◀── stop_token
   （生产者，唯一）                                  （消费者，可拷贝多份）
                                                        │
                                              stop_callback 注册
                                              （状态翻转时触发）
```

```cpp
#include <thread>
#include <stop_token>

std::jthread worker([](std::stop_token st) {
    while (!st.stop_requested()) {   // 消费者轮询
        do_one_iteration();
    }
    // 收到停止请求，清理后退出
});

std::this_thread::sleep_for(std::chrono::seconds{5});
worker.request_stop();   // 生产者发信号（jthread 内置 stop_source）
// worker 析构时还会自动 request_stop() + join()
```

## jthread 的停止令牌从哪来

```cpp
// 方式 1：lambda 第一个参数声明 stop_token（推荐）
std::jthread t1([](std::stop_token st) { /* ... */ });
// jthread 检测到 callable 接受 stop_token，自动把内部 token 传进去

// 方式 2：callable 不收 token，自己拿
std::jthread t2([] { /* 没有 st，无法响应停止 */ });
// t2 析构仍会 request_stop()+join()，但线程体不理会 → 等于阻塞等它跑完

// 手动拿 jthread 的 source/token：
std::stop_source src = worker.get_stop_source();
std::stop_token  tok = worker.get_stop_token();
```

**注意**：`request_stop()` 只是**协作式请求**——它不会 kill 线程。线程体必须自己检查 `stop_requested()` 并在合适点退出。卡在 `recv()` 里的线程照样卡着（见下文"打断阻塞调用"）。

## stop_token 的三个查询

```cpp
void work(std::stop_token st) {
    st.stop_requested();   // bool：是否已请求停止（轮询用）

    st.stop_possible();    // bool：有没有关联的 stop_source
                           // false → 永远不可能被停，连轮询都可以省掉
                           // （jthread 默认构造、token 默认构造时是 false）

    // 轮询的标准姿势：
    while (!st.stop_requested()) {
        // ...
    }
}
```

`stop_possible()==false` 时跳过所有检查，这对热路径有意义：**不可停止的线程零开销**。

## stop_callback：状态翻转瞬间执行

轮询有延迟（最坏一个循环周期），`stop_callback` 在 `request_stop()` 调用的**那一刻**同步执行：

```cpp
std::jthread replayer([](std::stop_token st) {
    int fd = open_market_feed();

    // 注册回调：收到停止请求立刻 close(fd)，把阻塞在 read 的线程踢醒
    std::stop_callback unblock(st, [fd] {
        ::close(fd);   // read() 立即返回 EBADF → 循环看到 stop_requested → 退出
    });

    while (!st.stop_requested()) {
        read_tick(fd);   // 阻塞点
    }
});

replayer.request_stop();   // 回调在 request_stop 内部同步跑完才返回
```

两个关键性质：

1. **回调在 `request_stop()` 的调用线程里执行**——回调里别拿调用方正持有的锁，否则死锁
2. 如果注册时**已经停止**，回调**立即**在当前线程执行（不会丢）

## 打断阻塞调用：条件变量也有 stop 版

`stop_callback + close(fd)` 是 socket 的打法；条件变量有标准解法：

```cpp
#include <condition_variable>

std::condition_variable_any cv;
std::mutex m;

// wait 的 stop_token 重载：收到停止请求会唤醒并返回 false
bool wait_data(std::stop_token st, Data& out) {
    std::unique_lock lk(m);
    return cv.wait(lk, st, [&] { return queue_has_data(); });
    // 停止请求 → 返回 false（不管谓词）；正常有数据 → 返回 true
}
```

`std::stop_token` 重载内部就是注册了一个临时 `stop_callback` 去 `notify_all`。

## 实战：行情消费线程的优雅停机

```cpp
class FeedConsumer {
    std::jthread th_;

    void run(std::stop_token st) {
        while (!st.stop_requested()) {
            auto tick = ring_.pop_for(std::chrono::milliseconds{100});
            if (!tick) continue;              // 超时空转 → 顺便检查 st
            process(*tick);                   // 热路径：无锁、无分配
        }
        flush_stats();                        // 停机收尾：落盘统计
    }

public:
    void start() { th_ = std::jthread([this](std::stop_token st) { run(st); }); }
    void stop()  { th_.request_stop(); }      // 或直接析构，自动 stop+join
};
```

设计要点：

- 热循环里**只查一次** `stop_requested()`（一次原子 load，可忽略）
- 阻塞点必须有超时或 stop 版 wait，否则停机要等食物
- 收尾逻辑（flush、统计落盘）放在循环后、`run` 返回前——RAII 停机正好路过

## 自测题

1. `request_stop()` 为什么不等价于 `pthread_cancel`？协作式停止对交易系统为什么是优点？
2. `stop_possible()==false` 的两种场景？这个查询对热路径的意义？
3. `stop_callback` 在哪条线程上执行？由此引出什么死锁风险？
4. 注册 `stop_callback` 时停止状态已翻转，会发生什么？这解决了什么竞态？
5. 为什么"阻塞在 read/recv 的线程"光靠 `stop_requested()` 轮询停不掉？给出两种解法（socket 场景、条件变量场景）。
