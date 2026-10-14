# 11 · EventLoop：one loop per thread 与跨线程唤醒

<a id="pnp-11-goal"></a>

## 目标

搞清楚 muduo 的线程模型为什么是 **one loop per thread**；别的线程怎么安全地给 IO 线程投递任务（`runInLoop` / `queueInLoop`）；为什么唤醒要用 `eventfd` 而不是条件变量。

<a id="pnp-11-unp"></a>

## UNP 对照

- UNP Ch16（非阻塞 I/O）、Ch26（线程）——书里罗列了几种并发模型（每连接一进程/线程、单线程 + IO 复用、领导者 follower 等），muduo 采用的是并进一步收紧的：**让"线程"和"事件循环"一对一绑定，而不是让多个线程争抢同一个循环**
- 本书 📖 陈硕《Linux 多线程服务端编程》**第 2 章（线程模型图）**、第 4 章（EventLoop）

## 概念详解

### 1. one loop per thread 的确切含义

不是"一个线程一个 epoll 实例"这么简单，而是三条硬约束：

| 约束 | 实现手段 | 违反后果 |
|------|----------|----------|
| 一个线程至多一个 EventLoop | 线程局部指针 `__thread EventLoop* t_loopInThisThread`，构造时若已非空直接 `LOG_FATAL` | 源码：`EventLoop.cc:29`、`EventLoop.cc:78–86` |
| 一个 EventLoop 只跑在创建它的那个线程 | `threadId_` 构造时记下 `CurrentThread::tid()`；`assertInLoopThread()` 到处拦 | 未定义行为、数据竞争 |
| 要改动 Connection/Channel，**必须回到它所属的 loop 线程去做** | `runInLoop` / `queueInLoop` 投递，而不是加锁直接改 | 竞态、悬垂 |

> 这条约束的工程价值是**去掉了绝大部分锁**：因为一个连接的生命周期事件都在自己的线程里处理，业务流程可以"假装是单线程的"。这也是 muduo 与"锁到处都是"的传统服务器最大的区别。

### 2. 跨线程投递：`runInLoop` 与 `queueInLoop`（源码：`EventLoop.cc:148–171`，已核对）

```cpp
void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) { cb(); }                 // 本线程：直接同步执行
  else                  { queueInLoop(std::move(cb)); }
}

void EventLoop::queueInLoop(Functor cb) {
  { MutexLockGuard lock(mutex_);
    pendingFunctors_.push_back(std::move(cb)); }
  // 两种情形才需要唤醒：① 跨线程投递；② 本线程正在跑 functors（防止新任务被漏掉）
  if (!isInLoopThread() || callingPendingFunctors_) { wakeup(); }
}
```

两个容易答错的细节：

1. **`queueInLoop` 即使在本线程调用也不一定白等**：如果此刻正在 `doPendingFunctors()` 里跑任务（`callingPendingFunctors_ == true`），新加的任务这一轮不会被看到，所以要补一次 `wakeup()`。
2. **锁保护的范围只有那个 `vector`**：任务本身在锁外执行。见下。

### 3. `doPendingFunctors`：`swap` 换出再执行（源码：`EventLoop.cc:254–269`）

```cpp
void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;
  { MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_); }        // 锁只覆盖一次 swap
  for (const Functor& functor : functors) functor();   // 执行期间不持锁
  callingPendingFunctors_ = false;
}
```

这是教科书级的**减小临界区**手法：

- 不这么做的话，执行 functor 期间一直持锁 → 别的线程投递全部阻塞；
- 更隐蔽的问题是：**functor 里再调 `queueInLoop` 会造成死锁**（同一把 `mutex_` 非递归）。`swap` 之后锁已释放，所以回调里再投递是安全的。

### 4. 为什么用 `eventfd` 唤醒，不用条件变量

源码：`EventLoop.cc:33–42, 73–74, 87–90, 234–242`

```cpp
int createEventfd() { return ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); }
// 构造函数里：
wakeupFd_(createEventfd()),
wakeupChannel_(new Channel(this, wakeupFd_)),
// ...
wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
wakeupChannel_->enableReading();          // "we are always reading the wakeupfd"

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);   // 写 8 字节
  if (n != sizeof one) LOG_ERROR << "... writes " << n << " bytes instead of 8";
}
```

理由（面试高频）：

| 方案 | 问题 |
|------|------|
| 条件变量 + bool flag | loop 正阻塞在 `epoll_wait` 上等 IO，条件变量救不了它——`epoll_wait` 看不见 pthread 的条件。**必须有一个 IO 事件才能打断 epoll_wait** |
| pipe | 需要两个 fd；`eventfd` 只要一个，语义恰好是"计数器"，内核开销更小 |
| `eventfd` ✅ | 既是 fd（可被 epoll 监听）又能当计数器，**多次 wakeup 合并成一次 `epoll_wait` 返回**，天然去重 |

顺手记：`wakeupFd_` 读到的 8 字节数据 discard 掉即可（`handleRead` 只读不处理）；累积计数不会溢出到 0 除非读了 2^64 次。

### 5. 每个 loop 的一次完整循环（源码：`EventLoop.cc:103–134`）

```
while (!quit_) {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs /*10000*/, &activeChannels_);
    ++iteration_;
    eventHandling_ = true;
    for (Channel* ch : activeChannels_) { currentActiveChannel_ = ch; ch->handleEvent(pollReturnTime_); }
    currentActiveChannel_ = NULL; eventHandling_ = false;
    doPendingFunctors();              // ★ IO 回调之后、下一轮 poll 之前执行
}
```

顺序很重要：**先处理 IO 事件，再执行待跑任务**。如果反过来，跨线程投递的任务会在 IO 之前插队，破坏"回调顺序 ≈ 事件顺序"的直观性。

### 6. 线程模型全景（本书 ch2）

```
                ┌── main EventLoop（Acceptor 所在）── accept 新连接 ──┐
                │                                                     ↓
                │                          round-robin 分发给 sub EventLoopThreadPool
                ↓                                                     │
TcpServer ─── baseLoop_                    SubReactor#0 ── EventLoop#0 ── N 个 TcpConnection
                                           SubReactor#1 ── EventLoop#1 ── N 个 TcpConnection
                                           ...（每个一个线程、一个 epoll）
```

- 连接建立后**归属固定**，不会在 loop 之间漂移 → 连接内部无需锁
- `TcpServer::newConnection` 里按 `eventLoopThreadPool_->getNextLoop()` 取下一个
- 如果线程池大小为 0，所有连接都在 `baseLoop_` 里跑（单线程模式，调试用）

## C++ 示例：40 行复刻「loop + eventfd 唤醒 + pending functor」

```cpp
// mini_loop.cpp — g++ -std=c++17 -O2 mini_loop.cpp -o mini_loop -lpthread
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdio>
#include <atomic>

class Loop {
public:
  Loop() {
    ep_ = epoll_create1(EPOLL_CLOEXEC);
    wfd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    epoll_event e{}; e.events = EPOLLIN; e.data.fd = wfd_;
    epoll_ctl(ep_, EPOLL_CTL_ADD, wfd_, &e);              // 常驻监听 wakeup fd
    tid_ = std::this_thread::get_id();
  }
  bool inLoopThread() const { return tid_ == std::this_thread::get_id(); }

  void runInLoop(std::function<void()> cb) {              // 对应 EventLoop::runInLoop
    if (inLoopThread()) cb(); else queueInLoop(std::move(cb));
  }
  void queueInLoop(std::function<void()> cb) {            // 对应 queueInLoop
    { std::lock_guard<std::mutex> g(mu_); pending_.push_back(std::move(cb)); }
    if (!inLoopThread() || doing_) wakeup();
  }
  void loop(int rounds) {
    std::vector<epoll_event> evs(16);
    for (int i = 0; i < rounds; ++i) {
      int n = epoll_wait(ep_, evs.data(), (int)evs.size(), 200);
      for (int k = 0; k < n; ++k)
        if (evs[k].data.fd == wfd_) { uint64_t one; ssize_t r = read(wfd_, &one, sizeof one); (void)r; }
      doPendingFunctors();
    }
  }
  size_t queueSize() { std::lock_guard<std::mutex> g(mu_); return pending_.size(); }
private:
  bool doing_ = false;
  void wakeup() { uint64_t one = 1; ssize_t n = write(wfd_, &one, sizeof one); (void)n; }
  void doPendingFunctors() {                              // swap 换出后再执行
    std::vector<std::function<void()>> fs;
    doing_ = true;
    { std::lock_guard<std::mutex> g(mu_); fs.swap(pending_); }
    for (auto& f : fs) f();
    doing_ = false;
  }
  int ep_, wfd_; std::thread::id tid_;
  std::mutex mu_; std::vector<std::function<void()>> pending_;
};

int main() {
  Loop* L = nullptr;
  std::thread io([&]{ Loop l; L = &l; l.loop(200); });
  while (!L) std::this_thread::yield();

  L->runInLoop([]{ printf("[IO线程] task A: 跨线程投递，第一次 runInLoop\n"); });
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  L->queueInLoop([]{ printf("[IO线程] task B: queueInLoop（即使同线程也排队）\n"); });
  printf("[main] 队列剩余=%zu（投递后立刻读，可能已被取走）\n", L->queueSize());
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  io.join();
}
```

## muduo 对应关系（读源码地图）

| 本文件元素 | muduo 组件 | 出处 |
|------------|-----------|------|
| `__thread EventLoop* t_loopInThisThread` | 同名线程局部指针 | `EventLoop.cc:29` |
| 一个线程一个 EventLoop 的断言 | 构造函数里的 `LOG_FATAL` | `EventLoop.cc:78` |
| `wakeupFd_` / `wakeupChannel_` | `eventfd` + `Channel` | `EventLoop.cc:73`、`87` |
| `wakeup()` 写 8 字节 | `EventLoop::wakeup` | `EventLoop.cc:234` |
| `pendingFunctors_` + `swap` | `doPendingFunctors` | `EventLoop.cc:254` |
| `runInLoop` / `queueInLoop` | 同名函数 | `EventLoop.cc:148` / `160` |
| 线程池与分发 | `EventLoopThreadPool` / `EventLoopThread` | `muduo/net/` |
| `poll()` 每轮超时 | `kPollTimeMs = 10000` | `EventLoop.cc:31` |
| 忽略 SIGPIPE | 文件级匿名对象 `IgnoreSigPipe` | `EventLoop.cc:45–56` |

## 内核视角

- `eventfd` 内部是一个 64 位计数器：`write` 累加，`read` 返回当前值并清零（`EFD_SEMAPHORE` 才是每次减 1）；非阻塞下计数器为 0 时 `read` 返回 `EAGAIN`
- 由于计数器存在，**多次 wakeup 在没来得及 read 之前会合并**，不会丢也不会排队成一堆事件
- `read(wakeupFd_)` 发生在 **loop 自己线程**，且必定有数据（level-triggered，写了一直可读到清）
- `eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)`：`CLOEXEC` 防止 fork/exec 泄露 fd，`NONBLOCK` 防止极端情况下阻塞写
- 关于延迟：跨线程投递 → `eventfd write`(~百 ns 级) → 唤醒 → `epoll_wait` 返回 → `doPendingFunctors`。**这条路径就是 microseconds 级**，HFT 里尽量把处理放在同一个线程，跨线程边界越少越好（见 HFT 关联）

## 坑点

- **`quit()` 不 wakeup 就退出不了**：`quit_ = true` 后若 loop 正睡在 `epoll_wait` 上，要等到下一次事件才醒；源码 `EventLoop.cc:136–146` 专门处理"跨线程 quit 要 wakeup"
- 别在 functor 里持 `mutex_`：`doPendingFunctors` 的 functors 已 swap 出锁外，但如果你在 functor 里再次 `queueInLoop`，注意 `callingPendingFunctors_` 为真会补一次 wakeup（正常行为，不是重复 bug）
- `runInLoop` 在本线程是**同步执行**：若回调很慢，会阻塞当次 loop 迭代 —— 长任务应丢给专门的线程池，而不是塞进 IO loop
- 每个 `EventLoop` 都有 `timerQueue_`（源码构造函数可见），定时器回调同样在 loop 线程执行
- `SOCK_CLOEXEC` / `EFD_CLOEXEC` 别忘了：多线程 + fork/exec 场景下 fd 泄露会导致"对端不知道连接已关"的经典事故

## HFT 关联

| 场景 | 关系 |
|------|------|
| 线程拓扑 | HFT 网关通常比 muduo 更极端：**绑核 + 单线程包干一段链路**（收包 → 解码 → 撮合 → 回包），零跨线程、零锁。muduo 的多 Reactor 是通用服务的好选择，但不是最低延迟模型 |
| 跨线程代价 | `runInLoop` = 一次 eventfd 写 + 唤醒 + 被调度到目前为止；这条路径在微秒量级（具体数值取决于负载、调度策略，需自己在目标机上量，**本文不给未实测的数字**）。结论方向是对的：热路径上宁可复制数据也不跨线程 |
| 闲置中断 | `kPollTimeMs = 10000` 这种长超时在 HFT 会放大尾延迟，通常改成 busy poll 或 `SO_BUSY_POLL`，见 [07 HFT 关联](./07_IO_epoll.md) |
| 观测 | 想量"每个 loop 迭代花了多久"：eBPF 抓 `epoll_wait` 的进出时间戳，见 [06.7 BPF 观测](../../06.7-bpf-observability/) |

## 自测题

1. `one loop per thread` 里的"三条硬约束"是什么，各自靠什么机制保证？
2. `runInLoop` 在本线程被调用时是同步还是异步？这带来什么风险？
3. 为什么需要一个 fd（`eventfd`）来唤醒，用条件变量不行吗？
4. `doPendingFunctors` 为什么要用 `swap` 把任务换出来再执行？直接持锁执行会怎样？
5. `queueInLoop` 里 `if (!isInLoopThread() || callingPendingFunctors_)` 后半保护的是什么场景？

<details>
<summary>参考答案</summary>

1. ① **一个线程至多一个 EventLoop**：靠 `__thread EventLoop* t_loopInThisThread`，构造时发现非空就 `LOG_FATAL`（`EventLoop.cc:29/78`）。② **EventLoop 只在创建它的线程 loop**：构造时记 `threadId_`，`assertInLoopThread()` 处处拦。③ **改动必须在所属 loop 线程做**：靠 `runInLoop`/`queueInLoop` 投递而非加锁。三条共同的结果是"业务可以假装单线程"，从而消掉绝大部分锁。
2. **同步执行**（`EventLoop.cc:148–153`：`if (isInLoopThread()) cb();`）。风险：回调耗时长会阻塞本轮 loop 迭代，所有其它连接都要等它；且回调里若再 `runInLoop` 会递归同步调用。所以重活应丢线程池，别塞 IO loop。注意这与 `queueInLoop` 的语义差别——后者保证"排队到下一轮"。
3. 条件变量救不了**阻塞在 `epoll_wait` 上的线程**：`epoll_wait` 等的是 IO 事件，不是 pthread 同步原语。必须有一个可被 epoll 监听的 fd 才能把它唤醒。选 `eventfd` 而非 pipe：只要一个 fd，且语义天然是"计数器"——**多次 wakeup 合并成一次 `epoll_wait` 返回**，不会浪费。
4. ① 缩小临界区：把 O(n) 的任务执行移出锁外，别的线程投递不会被长时间阻塞；② 避免死锁：functor 里再调用 `queueInLoop` 时，若还持着 `mutex_`（非递归）必然死锁；③ `swap` 之后锁已释放，回调里再投递是安全的。
5. 保护"**本线程正在跑 functors 时又收到新任务**"的场景。`doPendingFunctors` 执行期间 `callingPendingFunctors_ == true`，此时若本线程新 `queueInLoop` 一个任务，它进了 `pendingFunctors_` 但这一轮的 `swap` 已经发生过了，**这一轮看不见它**。若不补一次 wakeup，该任务要等到下一次有 IO 事件才被处理。所以条件里必须有 `|| callingPendingFunctors_`。

</details>

## 交叉引用

- 上一篇：[10 muduo Buffer](./10_MuduoBuffer.md) · 下一篇：[12 TcpConnection 生命期](./12_TcpConnectionLifetime.md)
- 对照：[07 I/O 复用](./07_IO_epoll.md) · [06 非阻塞 I/O](./06_NonBlockingIO.md) · [05 TTCP（吞吐）](./05_TTCP.md)
- 源码：`github.com/chenshuo/muduo` 的 `muduo/net/EventLoop.{h,cc}`、`muduo/net/EventLoopThreadPool.cc`（本文核对于 master 分支）
