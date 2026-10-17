# 10 · muduo Buffer：应用层缓冲区设计

<a id="pnp-10-goal"></a>

## 目标

非阻塞网络编程里 **为什么要自建应用层缓冲区**（而不是直接 read/write）；`inputBuffer_` / `outputBuffer_` 三个索引怎么动；`readFd` 为什么用 `readv` 配一个 64 KB 栈缓冲。

<a id="pnp-10-unp"></a>

## UNP 对照

- UNP Ch3.9（`readn`/`writen`）、Ch16（非阻塞）——UNP 的 I/O 函数假定"阻塞读到底"，而 Reactor + 非阻塞要求**一次事件读一批、粘由应用层处理**
- 本书 📖 陈硕《Linux 多线程服务端编程》**第 7 章** Buffer 设计

## 概念详解

### 1. 为什么必须有应用层 buffer

非阻塞 I/O 下，**单次 `read` 的字节数与"一条完整消息"没有任何关系**：可能来半条、可能来 3.5 条。（参见 [02 TCP 字节流](./02_TCPByteStream.md)）

因此每个 TCP 连接必须挂两块缓存：

| 缓存 | 谁写 | 谁读 | 作用 |
|------|------|------|------|
| `inputBuffer_` | `handleRead` → `readFd` | 业务 `onMessage` 回调 | 收半包/拆包；**业务一次没取完不会丢** |
| `outputBuffer_` | 业务 `send()` | `handleWrite` | 写不下去时（`EAGAIN`）暂存；**不能直接丢给用户自己去等** |

> muduo 的 `outputBuffer_` 是"高水位回调"存在的前提：当堆积超过 `highWaterMark`（默认 64 MB）触发回调通知业务限速。

### 2. 三索引布局（源码：`muduo/net/Buffer.h`）

```
        prependable bytes |  readable bytes  |  writable bytes
 begin()          readerIndex_        writerIndex_         end()
                  (readPtr)           (writePtr)
```

实测到的源码常量与成员初始化（`Buffer.h:45–51`，已核对）：

```cpp
static const size_t kCheapPrepend = 8;    // 8 字节头部预留
static const size_t kInitialSize  = 1024; // 初始可写 1024 字节
explicit Buffer(size_t initialSize = kInitialSize)
  : buffer_(kCheapPrepend + initialSize),   // 实际 vector 大小 = 8 + 1024
    readerIndex_(kCheapPrepend),
    writerIndex_(kCheapPrepend) {}
```

- **`kCheapPrepend = 8` 的意义**：让序列化时可以"往前面补长度头"。先写 body，最后 `prependInt32(header)` 填长度，避免为了写长度必须先知道长度而多做一次拷贝。**HFT 里这与"零拷贝组包"思路同源。**
- 用 `std::vector<char>` 而非裸 `new char[]`：自动扩容、自动释放，且 `hasWritten()` 只是一次整数加法。

### 3. `readFd`：`readv` + 64 KB 栈缓冲（源码：`Buffer.cc:25–57`，已核对）

```cpp
ssize_t Buffer::readFd(int fd, int* savedErrno) {
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin() + writerIndex_;  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;                vec[1].iov_len = sizeof extrabuf;
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0)                        *savedErrno = errno;
  else if (implicit_cast<size_t>(n) <= writable)  writerIndex_ += n;
  else {                            // 溢出到 extrabuf，走 append 拷贝
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}
```

这套设计的三个工程判断，面试最容易问：

1. **为什么不先 `ioctl(FIONREAD)` 查有多少字节再读？** 源码注释写死了原因：`saved an ioctl()/FIONREAD call` —— 一次额外系统调用的成本，比"偶尔多用点内存"更贵。
2. **为什么不笼统地"多给 Buffer 扩容"？** 缓冲区常见的稳态是小包、`writableBytes()` 足够；只有突发大流量才用得上 `extrabuf`。**用 64 KB 栈空间兜突发，避免每次 `read` 都撑大 `vector`**——这是"以栈换堆"的典型手法。
3. **为什么要 `readv` 两跳而不是拷 `extrabuf` 两次？** 一次系统调用拿上限 ault `writable + 65536 ≈ 128 KB-1`（源码注释原话：`when extrabuf is used, we read 128k-1 bytes at most`），省掉第二轮 `read` 的系统调用。

### 4. `makeSpace`：原地整理，而不是每次 `resize`

`Buffer.h:390–405` 的逻辑：写空间不够时，**先看"已读掉的部分 + 可写部分"够不够**，够就把 readable 数据整体搬到 `begin()+kCheapPrepend`（`std::copy`），腾出尾部；**实在不够才 `resize`**。

| 手段 | 触发条件 | 代价 |
|------|----------|------|
| 搬移（整理碎片） | `writable + prependable >= len + kCheapPrepend` | 一次 memmove/\`std::copy\`，无系统调用 |
| `resize` 扩容 | 上式不成立 | 可能内存分配 + 拷贝，**稳态下应避免** |

## C++ 示例：30 行复刻核心语义

```cpp
// mini_buffer.cpp — g++ -std=c++17 -O2 mini_buffer.cpp -o mini_buffer
#include <vector>
#include <cstring>
#include <algorithm>
#include <sys/uio.h>
#include <unistd.h>
#include <cstdio>

class Buffer {
public:
  static constexpr size_t kCheapPrepend = 8;
  static constexpr size_t kInitialSize  = 1024;

  explicit Buffer(size_t initial = kInitialSize)
    : buf_(kCheapPrepend + initial), r_(kCheapPrepend), w_(kCheapPrepend) {}

  size_t readableBytes()  const { return w_ - r_; }
  size_t writableBytes()  const { return buf_.size() - w_; }
  size_t prependableBytes() const { return r_; }
  const char* peek()      const { return begin() + r_; }

  void retrieve(size_t len) {                 // 业务消费掉 len 字节
    if (len < readableBytes()) r_ += len;
    else                       retrieveAll();
  }
  void retrieveAll() { r_ = w_ = kCheapPrepend; }

  void ensureWritableBytes(size_t len) {      // 对应 muduo 的 ensure+makeSpace
    if (writableBytes() < len) makeSpace(len);
    if (writableBytes() < len) buf_.resize(w_ + len);
  }
  void append(const char* d, size_t len) { ensureWritableBytes(len); std::memcpy(begin()+w_, d, len); w_ += len; }

  ssize_t readFd(int fd, int* savedErrno) {
    char extrabuf[65536];
    iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + w_;  vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;      vec[1].iov_len = sizeof extrabuf;
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    ssize_t n;
    do { n = ::readv(fd, vec, iovcnt); } while (n < 0 && errno == EINTR);
    if (n < 0) { *savedErrno = errno; return n; }
    if (static_cast<size_t>(n) <= writable)      w_ += n;
    else { w_ = buf_.size(); append(extrabuf, n - writable); }
    return n;
  }
private:
  char* begin() { return &*buf_.begin(); }
  const char* begin() const { return &*buf_.begin(); }
  void makeSpace(size_t len) {                // 整理碎片，尽量不扩容
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) return;
    size_t readable = readableBytes();
    std::copy(begin() + r_, begin() + w_, begin() + kCheapPrepend);
    r_ = kCheapPrepend; w_ = r_ + readable;
  }
  std::vector<char> buf_; size_t r_, w_;
};

int main() {
  Buffer b;
  printf("初始: read=%zu write=%zu prepend=%zu\n",
         b.readableBytes(), b.writableBytes(), b.prependableBytes());
  char buf[2] = {0};
  int err = 0;
  ssize_t n = b.readFd(STDIN_FILENO, &err); n += !!buf[0];   // 从标准输入读一段
  printf("读入 %zd 字节, read=%zu write=%zu\n", n, b.readableBytes(), b.writableBytes());
  b.retrieveAll();
  printf("retrieveAll 后: read=%zu write=%zu prepend=%zu（回到初始布局）\n",
         b.readableBytes(), b.writableBytes(), b.prependableBytes());
}
```

## muduo 对应关系（读源码地图）

| 本文件元素 | muduo 组件 | 出处 |
|------------|-----------|------|
| `Buffer` 类 | `muduo::net::Buffer` | `muduo/net/Buffer.h:45` |
| `readFd` 的 `readv` + 栈缓冲 | `Buffer::readFd` | `muduo/net/Buffer.cc:25` |
| `ensureWritableBytes` / `makeSpace` | 同名私有函数 | `Buffer.h:194` / `Buffer.h:390` |
| 8 字节头部预留 | `prependInt32` / `prependInt64` 等 | `Buffer.h:356` |
| 输入输出双缓冲 | `TcpConnection::inputBuffer_` / `outputBuffer_` | `muduo/net/TcpConnection.h` |
| 消费接口 | `retrieve*`、`peek*`、`readInt32` 系列 | `Buffer.h` |

## 内核视角

- `readv`（`readv(2)`）是一次系统调用填多个用户缓冲，内核按 iovec 顺序拷贝；**相比两次 `read`，省的不是拷贝量，而是系统调用进入/退出 + socket 锁获取的成本**
- 读取上限受 `SO_RCVBUF` 与接收队列实际数据量约束；一次读到 tonnes 128 KB-1 意味着**突发流量下不会让内核缓冲堆积**（堆积 → RTT 变大 → 延迟恶化）
- UDP/组播侧不能用这套：边界 haz durability 天然保留，见 [08 UDP/组播](./08_UDP_Multicast.md)

## 坑点

- **`retrieve` 也可能永远剩一点点**（半包），此时 `makeSpace` 会反复搬移 —— 长连接小包场景下建议 `retrieveAll` 与内在standing steady-state：缓冲最终会停在"读指针 = 写指针 = 8"的稳态，不再有搬移成本，这是 `retrieveAll` 复位的效果
- `readv` 返回 `(ssize_t)-1` 且 `errno == EINTR` 必须重试；`EAGAIN` 则是"没数据了"，要区分
- `extrabuf` 在栈上 64 KB：**每个 `readFd` 调用一个**。若在 deep recursion 或大量小栈线程上调用要小心（Reactor 单线程调用的场景没问题）
- `n > writable` 分支里 `writerIndex_ = buffer_.size()` 后调 `append`：`append` 内部会再走一次 `ensureWritableBytes`，**这是唯一可能触发 `resize` 的路径**
- 消费语义：`peek*`（只看）与 `retrieve*`（只推进读指针）必须分离——半包时不能撤回值，见 [09 序列化](./09_Serialization.md)
- `Buffer` 不是线程安全的：它只被**所属 EventLoop 那一个线程**触碰；跨线程要用 [11 的 `runInLoop`](./11_EventLoopThreading.md) 排队，不能加锁了事

## HFT 关联

| 场景 | 关系 |
|------|------|
| 行情/下单解码 | 组播或推流常一次来 N 条报文，`readv` 一次拿干净 → 减少系统调用次数。再往极致走就是环形缓冲（见 [14-hft ch07](../../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/)） |
| `kCheapPrepend` | 与"先组 body 再补长度头"的零拷贝组包同理；订单报文封装常用 |
| 内存确定性 | 稳态下 `Buffer` 不再扩容/搬移 → **延迟抖动收敛**，这是低延迟里比"平均更快"更重要的性质 |
| 警惕 | `extrabuf` 是**栈上** 64 KB：默认 8 MB 线程栈没问题，但在特意调小栈的线程或深调用路径里要算一下 |

## 自测题

1. 非阻塞 + epoll 场景下，为什么不能看完 [02](./02_TCPByteStream.md) 里那个 `readn` 直接用在业务里？
2. `readFd` 里为什么宁可用 `readv` + 64 KB 栈缓冲，也不用 `ioctl(FIONREAD)` 先问长度？
3. `kCheapPrepend = 8` 除了"留头部"，第二层用处是什么？
4. `makeSpace` 为什么不直接 `resize`？什么条件下还是会走到 `resize`？
5. `outputBuffer_` 存在的必要性是什么？"直接业务调 write、写不下重试"会有什么问题？

<details>
<summary>参考答案</summary>

1. `readn` 的语义是"**阻塞地**读满 n 字节才返回"。在 epoll + 非阻塞下，`read` 返回 `EAGAIN` 表示暂时没数据，若继续 `readn` 会① 空转烧 CPU；② 更糟的是消息长度本来就是变长的，你事先不知道 n。正确的是：**有数据就读一批进 `inputBuffer_`，由应用层按协议判断消息边界**，消费不完的数据留着等下一批凑整。
2. `ioctl(FIONREAD)` 是一次额外系统调用。每秒几十万次 event 的场景，"省掉一次 syscall"比"偶尔多用 64 KB 栈"划算得多——源码注释就是 `// saved an ioctl()/FIONREAD call to tell how much to read`。同时多段 `readv` 让单次 syscall 的上限提高到 ~128 KB，突发时不用反复进出内核。
3. 让序列化**事后补长度头**：可以先写 body，最后 `prependInt32(totalLen)` 把长度填到前面，省掉"必须先算长度再 memcpy 两遍"的开销（与之对比：没有 prepend 区就得临时缓冲或二次拷贝）。这跟 HFT 里"零拷贝组包"是一类思路。
4. `makeSpace` 优先把 readable 数据搬到 `kCheapPrepend` 位置（`std::copy`），**复用已读掉的空间**，避免长大 `vector` 导致分配 + 拷贝 + 后续所有访问的地址变化。只有当"可写 + 可 prepend 空间 < 需要的长度 + 8"、即碎片整理也不够时才走 `resize`——这是稳态下不该发生的路径；如果它频繁发生，说明业务消费太慢或单条消息大于阈值。
5. 非阻塞 `write` 可能**部分写**（只写出去一半）。没有 `outputBuffer_` 就必须让业务自己持锁/注册 `EPOLLOUT` 等待，等于把 I/O 状态管理丢给用户；而且"写不下就重试"会① 立刻返回忙等烧 CPU；② 无法保证多条 `send` 的顺序；③ 无法统计堆积量 → 没有高水位限速。muduo 由框架统一处理：有剩余就 `enableWriting()`，写完立刻 `disableWriting()`（对应 [07 的血泪坑](./07_IO_epoll.md)：`EPOLLOUT` 常驻注册会导致事件风暴）。

</details>

## 交叉引用

- 上一篇：[09 序列化](./09_Serialization.md) · 下一篇：[11 EventLoop 与线程模型](./11_EventLoopThreading.md)
- 对照：[02 TCP 字节流](./02_TCPByteStream.md) · [07 I/O 复用](./07_IO_epoll.md) · [14-hft ch07 无锁结构与内存布局](../../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/)
- 源码：`github.com/chenshuo/muduo` 的 `muduo/net/Buffer.h`、`muduo/net/Buffer.cc`（本文核对于 master 分支）
