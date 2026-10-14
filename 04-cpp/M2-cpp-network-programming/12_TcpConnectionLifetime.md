# 12 · TcpConnection 生命期：muduo 最容易被问崩的一段

<a id="pnp-12-goal"></a>

## 目标

回答一个听起来简单、实则决定成败的问题：**连接断开的瞬间，那个 `shared_ptr<TcpConnection>` 到底活到什么时候？** 以及 `Channel::tie()`、`shared_from_this()`、`runInLoop` 三件套是怎么协同防止"回调执行到一半对象没了"的。

<a id="pnp-12-unp"></a>

## UNP 对照

- UNP Ch5–6（`SIGPIPE`、半关闭）、Ch11/Ch14——UNP 里连接关闭是 `close(fd)` 一句话的事；C++ 里要处理的是**对象生命期与 fd 生命期的错配**
- 本书 📖 陈硕《Linux 多线程服务端编程》**第 1 章**（对象生命期）、第 8 章（TcpConnection）

## 概念详解

### 1. 为什么这件事必须用 `shared_ptr`，而且必须是 `enable_shared_from_this`

连接的死亡莫名其妙地难：它要被三样东西同时引用——

| 引用方 | 目的 |
|--------|------|
| `TcpServer::connections_` 容器 | 管理保活、统计、遍历 |
| `Channel`（挂在 epoll 上） | 事件到来时要回调它，回调期间它必须活着 |
| **用户回调**（`onMessage` 等） | 用户可能把 `conn` 存起来异步发数据 |

裸指针、或者手写 intrusive 引用计数，都解决不了"某处偷偷延长生命期"的问题。muduo 的做法是统一 `shared_ptr<const TcpConnection>`（`TcpConnectionPtr`），并在类内部需要"延长自己"的地方调 `shared_from_this()`——**所以 `TcpConnection` 必须继承 `enable_shared_from_this<TcpConnection>`**。

> 源码里 `shared_from_this()` 出现在这里：`connectEstablished`、`connectDestroyed`、`handleRead`/`handleWrite` 的回调、`forceClose` 等（详见地图表）。

### 2. 状态机（`muduo/net/TcpConnection.h`）

```
kConnecting --connectEstablished--> kConnected --shutdown--> kDisconnecting
     |                                  |                          |
     +--------------- handleClose ------+--------------------------+--> kDisconnected
```

`handleClose` 的入口 assert（`TcpConnection.cc:412`）：`state_ == kConnected || state_ == kDisconnecting`——**即"连接建立过"才能走关闭流程**，重复关闭会被 assert 抓到。

### 3. 建立：`connectEstablished`（源码：`TcpConnection.cc:323–332`，已核对）

```cpp
void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this());     // ★ 关键一行
  channel_->enableReading();
  connectionCallback_(shared_from_this());
}
```

`channel_->tie(shared_from_this())` 干了什么？看 `Channel.cc:47–51, 66–81`：

```cpp
void Channel::tie(const std::shared_ptr<void>& obj) { tie_ = obj; tied_ = true; }

void Channel::handleEvent(Timestamp receiveTime) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();                 // weak_ptr::lock —— 升级为 shared_ptr
    if (guard) handleEventWithGuard(receiveTime);   // 还活着才调回调
  } else {
    handleEventWithGuard(receiveTime);
  }
}
```

**`tie_` 是 `weak_ptr<void>`**：不改变所有权，只是"必要时把对象捞住"。事件到达时 `lock()`：

- 捞得到 → 回调期间 `guard` 持 strongly reference，对象安全；回调结束后 `guard` 析构，引用计数归零则对象析构；
- 捞不到（`lock()` 返回空）→ 说明对象已经在别处被销毁了，**直接跳过回调**，避免 use-after-free。

这就是 C++ 网络编程里"回调期间保活"的标准解法。注意 `tie_` 存成 `shared_ptr<void>` 是**为了抹掉类型**——`Channel` 不需要知道 `TcpConnection` 是什么，避免头文件依赖与循环依赖。

### 4. 关闭：`handleClose`（源码：`TcpConnection.cc:408–421`，已核对）

```cpp
void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());   // ★ 保命
  connectionCallback_(guardThis);
  // must be the last line
  closeCallback_(guardThis);
}
```

三处注释都是考点：

1. **为什么不在这里 `close(fd)`？** 源码注释：`leave it to dtor, so we can find leaks easily`——把 close 交给 `Socket` 的析构（RAII），这样凡是没被正确销毁的连接一定会留下 fd，用 `lsof` / `/proc/<pid>/fd` 一眼看到泄漏。
2. 为什么需要 `guardThis`？因为 `closeCallback_` 会让 `TcpServer` 从 `connections_` 里 erase，**那里可能是最后一个共享引用**——如果不先用局部 `guardThis` 抓住自己，`this` 在回调返回后就悬垂了。
3. 为什么 `closeCallback_` **必须是最后一行**？它之后不再触碰任何成员。`TcpConnectionPtr guardThis` 也是函数里最后析构的对象之一（局部变量逆序析构）。

### 5. 收尾：`connectDestroyed` 与 `TcpServer` 的销毁（源码已核对）

```cpp
void TcpConnection::connectDestroyed() {              // TcpConnection.cc:334-345
  loop_->assertInLoopThread();
  if (state_ == kConnected) {
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());
  }
  channel_->remove();                                 // 从 epoll 注销
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {   // TcpServer.cc:100
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}
// 注意 bind 里带着 conn（shared_ptr 拷贝），保证 removeConnectionInLoop 执行时对象仍存活

TcpServer::~TcpServer() {                             // TcpServer.cc:39-51
  loop_->assertInLoopThread();
  for (auto& item : connections_) {
    TcpConnectionPtr conn(item.second);               // 先抓一份
    item.second.reset();                              // 从容器摘掉
    conn->getLoop()->runInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));  // 送回它自己的线程销毁
  }
}
```

最后这段是**"跨线程销毁"的范式**：连接属于别的 IO 线程，绝不能在 `TcpServer` 所在线程直接析构（否则会出现"别的线程正在回调、这边已经析构"），要把所有权打包成 `shared_ptr` 送回它自己的 loop 线程再处理。

### 6. 一句话总结这条链路

```
对端 FIN / RST
  →  fd 可读 → epoll_wait 返回 → Channel::handleEvent
  →  tie_.lock() 成功（guard 出场）→ TcpConnection::handleRead
  →  inputBuffer_.readFd() 返回 0 → handleClose()
  →  TcpConnectionPtr guardThis(shared_from_this())  ← 保命
  →  closeCallback_ → TcpServer::removeConnection
  →  loop_->runInLoop(removeConnectionInLoop(conn))  ← 再保一次
  →  connections_.erase(conn) → 最后一个 shared_ptr 消失
  →  ~TcpConnection → ~Socket → close(fd)
```

## C++ 示例：手写一遍 tie/lock 保活

```cpp
// tie_guard.cpp — g++ -std=c++17 -O2 tie_guard.cpp -o tie_guard
#include <memory>
#include <cstdio>
#include <functional>

// ---- 模拟 muduo::net::Channel ----
class Channel {
public:
  void tie(const std::shared_ptr<void>& obj) { tie_ = obj; tied_ = true; }
  void handleEvent() {
    std::shared_ptr<void> guard;
    if (tied_) {
      guard = tie_.lock();               // 升级：活着才回调
      if (!guard) { printf("  [Channel] 对象已死，跳过回调（避免 UAF）\n"); return; }
    }
    cb_();                               // guard 的作用域覆盖整个回调
  }
  std::function<void()> cb_;
private:
  std::weak_ptr<void> tie_; bool tied_ = false;
};

// ---- 模拟 TcpConnection ----
struct Conn : std::enable_shared_from_this<Conn> {
  Channel ch;
  int id;
  explicit Conn(int i) : id(i) {}
  void establish() {
    auto self = shared_from_this();
    ch.tie(self);                        // weak_ptr，不影响所有权
    ch.cb_ = [self] { printf("  [Conn %d] 回调执行中，self.use_count=%ld\n",
                             self->id, self.use_count()); };
  }
};

int main() {
  auto c = std::make_shared<Conn>(1);
  c->establish();
  printf("tie 之后 use_count=%ld（tie 用 weak_ptr，不增加计数）\n", c.use_count());

  printf("正常回调：\n"); c->ch.handleEvent();

  c.reset();                              // 唯一强引用释放 → 对象析构
  printf("析构后再触发事件（模拟迟到的 epoll 事件）：\n");
  // 注意：真实代码里 Channel 也随之销毁，这里仅为演示 lock() 失败分支
  Conn* leaked = new Conn(2);
  leaked->establish();
  leaked->ch.handleEvent();
  delete leaked;
}
```

## muduo 对应关系（读源码地图）

| 本文件元素 | muduo 组件 | 出处 |
|------------|-----------|------|
| `tie(shared_from_this())` | `Channel::tie` | `Channel.cc:47`；调用点 `TcpConnection.cc:328` |
| `tie_.lock()` 保活 | `Channel::handleEvent` | `Channel.cc:66–81` |
| `TcpConnectionPtr guardThis` | `handleClose` 局部变量 | `TcpConnection.cc:417` |
| "fd 交给 dtor 关" | `handleClose` 注释 | `TcpConnection.cc:413` |
| 建立/销毁 | `connectEstablished` / `connectDestroyed` | `TcpConnection.cc:323` / `334` |
| `closeCallback_` 注册 | `TcpServer::newConnection` | `TcpServer.cc:96`（源码自带 `// FIXME: unsafe`） |
| 移除连接 | `removeConnection` / `removeConnectionInLoop` | `TcpServer.cc:100` / `106` |
| 跨线程销毁 | `TcpServer::~TcpServer` | `TcpServer.cc:39–51` |

## 内核视角

- 不主动 `close(fd)` 不等于不关：`Socket` 对象是 RAII 的，析构里会 `sockets::close(sockfd_)`。所以"fd 泄漏 = 对象没析构"，数 `/proc/<pid>/fd` 就能直接验证
- `channel_->remove()` 必须在对象析构前完成：**先 `EPOLL_CTL_DEL` 再从 epoll 注销**，否则 epoll 里残留一个指向已释放对象的 `Channel*`（见 [07 的 fd 复用坑](./07_IO_epoll.md)）
- `RST` 与 `FIN` 的差异在本层不可见——`read` 都返回差别：FIN → read 返回 0；RST → read 返回 -1 且 `ECONNRESET`（走 `handleError` 而不是 `handleClose`）

## 坑点

- **`enable_shared_from_this` 的调用时机**：构造期间对象还没被 `shared_ptr` 接管，此时调 `shared_from_this()` 会抛 `bad_weak_ptr`。muduo 把 `tie` 放在 `connectEstablished`（已被 `shared_ptr` 管理之后）而不是构造函数里
- **`tie` 之后回调里千万别再 `tie` 自己形成循环引用**：`tie_` 是 `weak_ptr`，正是为了这条
- **`handleClose` 之后不要再触碰任何成员**：源码注释 `must be the last line` 就是警告
- **跨线程析构**：不要在 A 线程里直接 `delete`/释放属于 B 线程 loop 的连接，用 `runInLoop` 送回去（`TcpServer::~TcpServer` 的写法）
- 用户回调（`onMessage`）里若异步保存了 `shared_ptr` 并发队列逻辑，请务必同时保存 `shared_ptr` 而不是裸 `this`——这是 muduo 用户代码最常见的 UAF

## HFT 关联

| 场景 | 关系 |
|------|------|
| `shared_ptr` 的代价 | 每次拷贝/析构一次原子 RMW + 控制块间接跳转。muduo 用它是为了**连接的通用性**；HFT 热路径里连接数固定、生命周期常在系统启停边界，通常用**定长数组 + 索引 + pooled 对象**完全避开引用计数（见 [04 M1 条款 18–22](../M1-modern-cpp/01-Effective-Modern-C%2B%2B/README.md)） |
| 回调保活 | 低延迟代码里连 `weak_ptr::lock()` 的原子操作都想省掉，改用"连接表 + 世代号（generation）"：先查表校验版本号再进回调 |
| 延迟预算 | 断连处理不在热路径，可以保留 `shared_ptr` 的安全写法。**别为了微优化把冷路径写成 UAF**——这是工程师判断力的分水岭 |
| 排查 | 连接泄漏 ⟹ fd 数单调增长，`ls /proc/<pid>/fd \| wc -l` 一秒看出来；这正是陈硕特意不在 `handleClose` 里关闭 fd 的原因 |

## 自测题

1. `Channel::tie()` 存的为什么是 `weak_ptr<void>` 而不是 `shared_ptr<TcpConnection>`？
2. 事件到达时 `tie_.lock()` 失败（返回空）意味着什么？此时 muduo 怎么处理？
3. `handleClose` 里为什么要 `TcpConnectionPtr guardThis(shared_from_this())`？去掉会怎样？
4. 为什么源码特意不在 `handleClose` 里 `close(fd)`？
5. 为什么 `TcpServer::~TcpServer` 不直接销毁连接，而要 `runInLoop` 丢给各连接自己的 loop？
6. `enable_shared_from_this` 在构造函数里调用会发生什么？

<details>
<summary>参考答案</summary>

1. 两个原因：其一，**所有权语义**——`Channel` 不应该拥有 `TcpConnection`（拥有者是 `TcpServer::connections_` 以及用户），若存 `shared_ptr` 就形成引用计数环路（Channel 属于 Conn，Conn 指向 Channel），永远析构不掉；`weak_ptr` 不增加引用计数。其二，**解耦**——`weak_ptr<void>` 擦掉了类型，`Channel` 无需 include `TcpConnection.h`，避免头文件循环依赖。
2. `lock()` 返回空意味着**对象已经在其它路径被销毁**（引用计数已经归零），而这是一个迟到的、陈旧的事件。muduo 直接**跳过回调**（`Channel.cc:66–81` 里 `if (guard)` 才执行），从而避免了 use-after-free。典型触发场景：fd 被 close 后又被 dup/fork 保留引用导致通知照发，见 [07 的坑点](./07_IO_epoll.md)。
3. `closeCallback_` 会被 `TcpServer::removeConnection` → `removeConnectionInLoop` 处理，**而它可能持有这个连接的最后一个 `shared_ptr`**。如果不用局部 `guardThis` 先抓住自己，那么回调返回的瞬间 `this` 就被析构，后续任何成员访问（含析构函数里）都是 UAF。所以：保命 → 通知用户 → `closeCallback_` 必须是最后一行。
4. 源码注释原话：`we don't close fd, leave it to dtor, so we can find leaks easily`。把 fd 的关闭放进 `Socket` 的析构（RAII），那么任何没被正确销毁的连接一定会留下一个 fd。泄漏检测从"看代码"变成"数 `/proc/<pid>/fd`"，这在长跑的服务器上是巨大的可观测性收益。
5. 因为连接属于**另一个 IO 线程**的 loop，在那个线程可能正在执行它的回调（或者 epoll 里还挂着它的 `Channel`）。在 `TcpServer` 所在线程直接销毁会造成跨线程析构的数据竞争。正确做法（`TcpServer.cc:39–51`）：`TcpConnectionPtr conn(item.second)` 抓一份所有权 → `item.second.reset()` 从容器摘掉 → `conn->getLoop()->runInLoop(bind(connectDestroyed, conn))`，把析构动作连同 `shared_ptr` 一起送回它自己的线程执行。
6. 抛 `std::bad_weak_ptr` 异常（C++17 起）或未定义行为。因为此时对象还没有被任何 `shared_ptr` 管理，内部的 `weak_this` 是空的。所以只能在"已被 `shared_ptr` 接管之后"（如 `connectEstablished`）调用 `shared_from_this()`，这也是 muduo 把 `tie` 放到 `connectEstablished` 而不是构造函数里的原因。

</details>

## 交叉引用

- 上一篇：[11 EventLoop 与线程模型](./11_EventLoopThreading.md) · 下一篇：[13 异步日志](./13_AsyncLogging.md)
- 对照：[10 muduo Buffer](./10_MuduoBuffer.md) · [03 自连接](./03_SelfConnect.md) · [07 I/O 复用](./07_IO_epoll.md)
- C++ 基础：[M1 智能指针](../M1-modern-cpp/01-Effective-Modern-C%2B%2B/README.md) · [M3 对象模型](../M3-deep-principles/01-Cpp-Object-Model/)
- 源码：`github.com/chenshuo/muduo` 的 `muduo/net/TcpConnection.cc`、`Channel.cc`、`TcpServer.cc`（本文核对于 master 分支）
