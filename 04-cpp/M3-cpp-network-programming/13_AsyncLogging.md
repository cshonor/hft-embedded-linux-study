# 13 · 异步日志：双缓冲 + 后台线程落地

<a id="pnp-13-goal"></a>

## 目标

理解 muduo 日志系统为什么**异步**、为什么需要**四个 buffer**、以及"日志打不出来"和"日志把进程拖垮"这两种失败如何被同时避免。

<a id="pnp-13-unp"></a>

## UNP 对照

- UNP Ch13（守护进程与日志）——传统 syslog / 同步写盘
- 本书 📖 陈硕《Linux 多线程服务端编程》**第 5 章**（高效的异步日志）

## 概念详解

### 1. 为什么业务线程不能直接 `fprintf`

| 做法 | 问题 |
|------|------|
| 业务线程直接写文件 | `write` 是系统调用，可能因为磁盘抖动阻塞**毫秒级**——比网络往返还慢，直接污染延迟分布 |
| 每次加互斥锁写 | 多线程串行化到一次 syscall 上，吞吐崩 |
| `O_APPEND` 慷慨拼内存 | 缓冲后再写则需要额外的buffer管理 |

muduo 的解法：**业务线程只做内存追加（append），由一个专门的后台线程负责写盘**。业务线程遭遇的最坏情况只是"一次 memcpy + 一次可能的 notify"。

### 2. 四个 buffer（源码：`AsyncLogging.cc:25–31`，已核对）

```cpp
AsyncLogging::AsyncLogging(...)
  : ..., currentBuffer_(new Buffer), nextBuffer_(new Buffer), buffers_() {
  currentBuffer_->bzero();  nextBuffer_->bzero();
  buffers_.reserve(16);
}
```

| 角色 | 位置 | 作用 |
|------|------|------|
| `currentBuffer_` | 前台 | 业务线程正在往里 append |
| `nextBuffer_` | 前台备胎 | `currentBuffer_` 满了立刻顶上，**避免临时 new** |
| `buffers_`（vector） | 前后台交接 | 攒着装满的 buffer，后台线程来取 |
| `newBuffer1` / `newBuffer2` | 后台持有 | 后台线程自己准备的两个空 buffer，用来"回填"前台 |

`newBuffer1/2` 定义在 `threadFunc` 内部（`AsyncLogging.cc:63–65`）：**把新 buffer 的准备放在后台线程**，前台在切换时就不需要 `new`（也即不需要承担内存分配的不确定性）。

### 3. 前台路径：`append`（源码：`AsyncLogging.cc:34–56`，已核对）

```cpp
void AsyncLogging::append(const char* logline, int len) {
  muduo::MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len) {
    currentBuffer_->append(logline, len);          // 快路径：纯 memcpy
  } else {                                          // 慢路径：换 buffer
    buffers_.push_back(std::move(currentBuffer_));  // 满的排进队列
    if (nextBuffer_) currentBuffer_ = std::move(nextBuffer_);
    else             currentBuffer_.reset(new Buffer);   // Rarely happens
    currentBuffer_->append(logline, len);
    cond_.notify();
  }
}
```

- 默认 `kLargeBuffer = 4 MB`（`AsyncLogging.h`），所以正常情况下十几次 append 才触发一次换 buffer
- **`Rarely happens` 那一行是亲手痕：后台线程来不及补 `nextBuffer_` 才走 `new`。** 这正是为什么要有 `newBuffer1/2` 这套机制——把罕见情形兜住，而不是让它成为常态
- 注意：只有慢路径才 `notify()`，快路径不打扰后台线程

### 4. 后台路径：`threadFunc`（源码：`AsyncLogging.cc:58–135`，已核对）

后台线程一轮循环做的事：

```cpp
{
  MutexLockGuard lock(mutex_);
  if (buffers_.empty()) cond_.waitForSeconds(flushInterval_);   // 最多等 3 秒
  buffers_.push_back(std::move(currentBuffer_));   // 把没满的也收走（保实时性）
  currentBuffer_ = std::move(newBuffer1);          // 回填空 buffer
  buffersToWrite.swap(buffers_);                   // ★ 一次 swap 拿走全部
  if (!nextBuffer_) nextBuffer_ = std::move(newBuffer2);
}                                                   // 锁在这里就释放了
// 之后所有写盘动作都不持锁
if (buffersToWrite.size() > 25) { /* 丢弃超出的，打 "Dropped log messages" 警告 */ }
for (const auto& b : buffersToWrite) output.append(b->data(), b->length());
if (buffersToWrite.size() > 2) buffersToWrite.resize(2);   // 只保留 2 个用于复用
...回收 newBuffer1/2...
output.flush();
```

三个设计点：

1. **`swap` 而不是逐个 pop**：临界区压缩到 O(1)，和 [11 `doPendingFunctors`](./11_EventLoopThreading.md) 是完全同一个套路
2. **除了"写满"还有定时兜底**：即使没满，`waitForSeconds(flushInterval_)`（默认 3 秒）也会把 `currentBuffer_` 收走，保证日志的实时性
3. **主动丢弃策略**：单次超过 25 个 buffer（≈100 MB）就丢弃中间部分并打 `Dropped log messages at ... larger buffers` 到 stderr——**宁可丢日志也不让进程被磁盘拖死**。这是服务端系统里很关键的价值判断

### 5. 落地：`LogFile` + `AppendFile`（源码：`LogFile.cc`，已核对）

```
AsyncLogging::threadFunc → LogFile::append_unlocked → FileUtil::AppendFile::append → fwrite_unlocked
                         → LogFile::flush          → FileUtil::AppendFile::flush  → fflush
```

- `AsyncLogging` 传给 `LogFile` 的 `threadSafe=false`（`AsyncLogging.cc:62`）:因为只有一个后台线程在用它，**省掉每次 append 的加锁**
- 滚动规则在 `LogFile::append_unlocked`（`LogFile.cc:64–91`）：超过 `rollSize_` 就 roll；否则累计 `checkEveryN_` 次后检查是否跨天 / 是否需要 flush
- 文件名格式 `basename.YYYYmmdd-HHMMSS.hostname.pid.log`（`LogFile.cc:110–132`），用 GM 时间

### 6. 整个数据流

```
业务线程（N 条）                    后台线程（1 条）
LOG_INFO << ...  → LogStream::FixedBuffer
                 → AsyncLogging::append       ──┐
                     currentBuffer_ 满了:        │ buffers_（vector）
                     buffers_.push + swap?next   │
                     cond_.notify()            ─┼─> waitForSeconds(3s) 醒
                                                │   buffersToWrite.swap(buffers_)
                                                │   ↓ 全部写盘
                                                └   LogFile → file flush
```

## C++ 示例：最小双缓冲异步日志骨架

```cpp
// mini_asynclog.cpp — g++ -std=c++17 -O2 mini_asynclog.cpp -o mini_asynclog -lpthread
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <atomic>

using Buffer = std::vector<char>;
using BufferPtr = std::unique_ptr<Buffer>;

class AsyncLog {
public:
  AsyncLog(size_t cap = 1024)
    : cap_(cap),
      cur_(new Buffer()), next_(new Buffer()),
      bg_([this]{ threadFunc(); }) {
    cur_->reserve(cap_); next_->reserve(cap_);
    full_.reserve(16);
  }
  ~AsyncLog() { quit_ = true; cv_.notify_all(); bg_.join(); }

  void append(const char* line, size_t len) {          // 前台：只做 memcpy
    std::unique_lock<std::mutex> lk(mu_);
    if (cur_->size() + len <= cap_) {
      cur_->insert(cur_->end(), line, line + len);      // 快路径
      return;
    }
    full_.push_back(std::move(cur_));                   // 慢路径：换 buffer
    if (next_) { cur_ = std::move(next_); cur_->clear(); }
    else       { cur_.reset(new Buffer()); cur_->reserve(cap_); }
    cur_->insert(cur_->end(), line, line + len);
    lk.unlock();
    cv_.notify_one();
  }
private:
  void threadFunc() {
    BufferPtr b1(new Buffer()), b2(new Buffer());        // 后台自备两个空 buffer
    b1->reserve(cap_); b2->reserve(cap_);
    std::vector<BufferPtr> toWrite; toWrite.reserve(16);
    while (!quit_) {
      {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait_for(lk, std::chrono::seconds(3));
        if (cur_ && !cur_->empty()) { full_.push_back(std::move(cur_)); }
        if (b1) { cur_ = std::move(b1); cur_->clear(); }
        toWrite.swap(full_);                             // O(1) 拿走全部
        if (!next_ && b2) { next_ = std::move(b2); next_->clear(); b2.reset(new Buffer()); b2->reserve(cap_); }
      }                                                  // 锁到此释放，写盘不持锁
      for (const auto& b : toWrite) fwrite(b->data(), 1, b->size(), stdout);
      fflush(stdout);
      // 回收两个用于下次回填（简化：直接留着）
      if (!b1 && !toWrite.empty()) { b1 = std::move(toWrite.back()); toWrite.pop_back(); b1->clear(); }
      if (!b2 && !toWrite.empty()) { b2 = std::move(toWrite.back()); toWrite.pop_back(); b2->clear(); }
      toWrite.clear();
    }
  }
  size_t cap_;
  BufferPtr cur_, next_;
  std::vector<BufferPtr> full_;
  std::mutex mu_; std::condition_variable cv_;
  std::atomic<bool> quit_{false};
  std::thread bg_;
};

int main() {
  AsyncLog log(256);
  for (int i = 0; i < 500; ++i) {
    char b[64];
    int n = snprintf(b, sizeof b, "log line %05d %s\n", i, std::string(40, '.').c_str());
    log.append(b, n);
  }
  std::this_thread::sleep_for(std::chrono::seconds(4));  // 等后台 flush
}
```

## muduo 对应关系（读源码地图）

| 本文件元素 | muduo 组件 | 出处 |
|------------|-----------|------|
| `cur_ / next_ / full_` | `currentBuffer_ / nextBuffer_ / buffers_` | `AsyncLogging.cc:25` |
| `append` 快慢路径 | `AsyncLogging::append` | `AsyncLogging.cc:34` |
| `newBuffer1/2` | `threadFunc` 内局部变量 | `AsyncLogging.cc:63` |
| `swap` 交接 | `buffersToWrite.swap(buffers_)` | `AsyncLogging.cc:83` |
| 丢弃策略 | `size() > 25` 分支 | `AsyncLogging.cc:92` |
| 定时 flush | `cond_.waitForSeconds(flushInterval_)` | `AsyncLogging.cc:79` |
| 单线程免锁写文件 | `LogFile output(..., false)` + `append_unlocked` | `AsyncLogging.cc:62` / `LogFile.cc:64` |
| 滚动与文件名 | `LogFile::rollFile` / `getLogFileName` | `LogFile.cc:93` / `110` |

## 内核视角

- 真正的 write 开销在后台线程：**页面 cache 命中时 `write` 很快，脏页回写（pdflush）或磁盘抖动时才慢**——把这一步隔离出前台，正是异步日志的全部意义
- `flush()`（`fflush`）不是必需的持久化手段（那是 `fsync`）；muduo 默认只用 `fflush`，靠 Linux page cache 保证，进程崩溃一般不会丢（操作系统还在），只有掉电才可能丢最近数据
- 想量化"日志到底拖了多少前台延迟"：eBPF 抓前台线程的调度/系统调用耗时，见 [06.7 BPF 观测](../../06.7-bpf-observability/)
- 缓冲区 `bzero()`（`AsyncLogging.cc:29`）的目的：**预取页**，让首次写入不触发缺页中断

## 坑点

- **进程异常退出会丢最后一批日志**：没来得及 flush 的 buffer 在内存里。重要场景要显式清理或用更高可靠性的方案
- **别把前后台之间的 `vector` 遍历放进锁**：`swap` 是关键；一旦在锁里遍历 + 写盘，前台 append 会被卡住
- `nextBuffer_` 为空时的 `new Buffer` 分支（`Rarely happens`）：如果它真的频繁发生，说明后台线程被磁盘拖慢，要检查落盘目标（NFS/慢盘会直接暴露）
- 日志内容自身的线程安全：`LogStream` 是每个业务线程自己的栈对象，**跨线程不宜共享同一个 stream**
- 丢弃日志的分支会擦除 `buffersToWrite` 中除前 2 个以外的所有 buffer（源码：`erase(begin()+2, end())`），这是有意的取舍

## HFT 关联

| 场景 | 关系 |
|------|------|
| 为什么 HFT 直接不上异步日志 | 异步日志仍需 memcpy + 偶尔的 mutex/notify——**仍有抖动**。极致做法是：热路径只写**环形缓冲 + 索引**（无锁、无分配、无系统调用），由专门的采集线程/进程 dump，甚至事后离线重放 |
| 可借鉴的部分 | 「前台不持锁做慢操作」「`swap` 缩小临界区」「新资源由后台线程预分配」这三条，在低延迟代码里同样成立，是通用工程手法 |
| 延迟预算 | 日志**不应计入撮合的关键路径**：测量时把它单独算一环，别让它成为 p99 报表里的隐藏项 |
| 观测替代 | HFT 常用采样 + 直方图埋点替代逐笔文本日志——热路径上格式化一行字符串的成本就够撮合几次了 |

## 自测题

1. 为什么要 `nextBuffer_` 和后台的 `newBuffer1/2` 一共 4 个 buffer，两个不够吗？
2. 后台线程为什么要 `swap` 而不是逐个拷贝 buffer？
3. 日志堆积过多时 muduo 会怎么做？为什么这么设计？
4. 为什么 `LogFile` 那边传 `threadSafe = false`？
5. 程序 crash 时，最后一批日志会怎样？异步日志的可靠性边界是什么？
6. `flushInterval_`（默认 3 秒）解决什么问题？

<details>
<summary>参考答案</summary>

1. 两个不够，因为要让**前台永远不需要临时 `new`**。`currentBuffer_` 满了时，立刻用 `nextBuffer_` 顶上（移动语义，O(1)）；后台线程还回来时也一次补两个（`newBuffer1/2`）。只有"后台线程也来不及补"的罕见情况才走到 `new Buffer`（源码注释 `Rarely happens`）。这套设计把内存分配从前台的高频路径上彻底挪走了——否则日志本身会带来 cmalloc 抖动。
2. `swap` 是 O(1) 指针交换，能把临界区压到最短；逐个拷贝/逐个 pop 要么贵、要么在锁里做多步操作。这样**所有写盘动作都在锁外执行**，前台线程几乎不受后台磁盘抖动的影响。（与 [11 `doPendingFunctors`](./11_EventLoopThreading.md) 的手法完全一致。）
3. 单次超过 25 个待写 buffer（约 100 MB）时，丢弃 `buffersToWrite` 中除前 2 个以外的所有内容，并往 stderr 和日志文件打一条 `Dropped log messages at ... larger buffers` 警告。这是明确的价值取舍：**宁可丢日志，也不让进程因为磁盘跟不上而被拖死**——服务端系统的可用性优先于日志完整性。
4. 因为写文件的只有 `AsyncLogging` 的这一个后台线程，`LogFile` 自身再加锁纯属浪费（`AsyncLogging.cc:62` 构造时传的就是 `false`），走 `append_unlocked` 不加锁路径。
5. 会丢。留在 `currentBuffer_` / `buffers_` 里还没落盘的内容随进程一起消失。边界是：**操作系统不崩，fflush 之后数据已在内核 page cache，通常不丢；进程自己崩掉，则可能丢最近几秒**。所以需要高可靠审计的场景不能只靠它（要追加 fsync 或独立日志进程）。
6. 解决"日志量太小导致实时性差"的问题：如果只有满了才写，低流量时段可能几分钟看不到一条日志（排查时极其难受）。`waitForSeconds(flushInterval_)` 到期后收走未写满的 `currentBuffer_`，保证最多 3 秒延迟。

</details>

## 交叉引用

- 上一篇：[12 TcpConnection 生命期](./12_TcpConnectionLifetime.md) · 本篇为 M2 muduo 三条主线的收尾
- 对照：[11 EventLoop 与线程模型](./11_EventLoopThreading.md)（`swap` 缩小临界区的同一手法） · [10 muduo Buffer](./10_MuduoBuffer.md)
- 工程侧：[01.5 CMake 构建](../../01.5-cmake-build/) · [03.6 用户态调试](../../03.6-userspace-debugging/) · [06.7 BPF 观测](../../06.7-bpf-observability/)
- 源码：`github.com/chenshuo/muduo` 的 `muduo/base/AsyncLogging.cc`、`muduo/base/LogFile.cc`、`muduo/base/LogFile.h`（本文核对于 master 分支）
