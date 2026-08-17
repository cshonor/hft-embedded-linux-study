# 02 · TCP 字节流与粘包

<a id="pnp-02-goal"></a>

## 目标

理解 **TCP 无消息边界**：一次 `send` 不等于一次 `recv`；应用层须定界（长度前缀、分隔符、固定长）。

<a id="pnp-02-unp"></a>

## UNP 对照

- [1.2 客户端示例](../../03.5-unix-network-api/1_BasicFoundation/Chapter01_Introduction/1.2_SimpleTimeClient.md)
- Ch3 `readn` / `writen`（本仓库 Ch3 节笔记）

<a id="pnp-02-concepts"></a>

## 概念详解

### 1. "粘包"不是 bug，是 TCP 的本义

TCP 提供的是 **字节流（byte stream）** 语义，只保证：字节不丢、不乱、不重。**消息边界根本不存在于协议中**——"粘包/半包"是应用层视角的错觉：

- 发 100 字节，对端第一次 `read` 可能拿到 40，也可能拿到 140（把你下一次的消息"粘"进来了）
- 原因在三层都可能发生：
  - **发送端**：Nagle 算法把多个小 `write` 合并进一个报文段
  - **网络**：中间设备分段/合并没有意义，但到达顺序与发送的分段不对应
  - **接收端**：应用读得太慢，数据在接收缓冲里排队，下次 `read` 一锅端

结论：**定界是协议设计的事，不是 TCP 的事。**

### 2. 三种定界方案对比

| 方案 | 典型协议 | 优点 | 缺点 |
|------|----------|------|------|
| 固定长度 | 早期行情帧、FIX 某些字段 | 解析最简单 | 浪费带宽、不灵活 |
| 分隔符 | HTTP 头（`\r\n`）、Redis（`\r\n`） | 可读、可流式 | 内容要转义、要扫描 |
| **长度前缀（TLV）** | 自定义协议、protobuf RPC 帧 | 一次 `read` 长度字段即知大小，分配精确缓冲 | 需要先读出头部（半头部问题） |

工程主流是 **长度前缀**：`[4B length][payload]`，解码器先凑齐 4 字节头，再等齐 payload。muduo 的 `LengthFieldCodec`、绝大多数交易网关协议都是这个思路。

### 3. `readn` / `writen`：把"最多 n"变成"正好 n"

`read(fd, buf, n)` 语义是 **最多** 读 n；协议解码需要"正好 n"的读法（UNP Ch3）：

```cpp
// 返回 <0 出错（EINTR 重试）；==0 对端 EOF；>0 实际读到的字节数（==n）
ssize_t readn(int fd, void* buf, size_t n) {
    size_t left = n;
    char*  p    = static_cast<char*>(buf);
    while (left > 0) {
        ssize_t r = read(fd, p, left);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) break;          // EOF
        left -= r; p += r;
    }
    return n - left;
}

// write 同理：可能部分写（发送缓冲满）
ssize_t writen(int fd, const void* buf, size_t n) {
    size_t left = n;
    const char* p = static_cast<const char*>(buf);
    while (left > 0) {
        ssize_t r = write(fd, p, left);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        left -= r; p += r;
    }
    return n;
}
```

注意 `write` 也要循环：**发送缓冲区满时 write 只写入一部分**（阻塞模式下）或返回 `EAGAIN`（非阻塞模式，见 [06](./06_NonBlockingIO.md)）。

### 4. muduo::net::Buffer 的设计（陈硕，重点）

muduo 用 `Buffer` 类解决粘包的读半边，布局：

```
+-------------------+------------------+------------------+
| prependable bytes |  readable bytes  |  writable bytes  |
+-------------------+------------------+------------------+
0               readerIndex_      writerIndex_          size()
```

三个设计决策：

1. **前置空隙（默认 8B+）**：读到长度前缀后，可以 **原地** 在数据前面补内容（如补长度头再转发），不用挪数据
2. **`readv` + 栈上 extra buf（64KB）**：一次系统调用尽量多读——读满 Buffer 剩余空间的同时，溢出部分先进栈缓冲，再决定要不要扩容，**避免每个连接预留 64KB 造成内存浪费**
3. **懒收缩**：数据取走后不急着 memcpy 压缩，readerIndex 前移即可，写满时再统一整理

写半边由 `TcpConnection::send()` 处理：先尝试直接 `write`，写不完的剩余部分挂到 Buffer，等 EPOLLOUT（见 [07 epoll](./07_IO_epoll.md)）。

### 5. 长度前缀解码器的状态机

```
[等头部4B] --读满4B, 校验len合法--> [等payload len B] --读满--> 回调 onMessage, 回到初始
```

必须校验 `len` 上限（如 64MB）——恶意/错误的长度值会让 `resize` 直接把进程 OOM。

<a id="pnp-02-code"></a>

## C++ 示例：长度前缀编解码

```cpp
// 编码：[4B 大端长度][payload]
void sendFrame(int fd, const std::string& payload) {
    uint32_t len = htonl(static_cast<uint32_t>(payload.size()));
    std::string frame(reinterpret_cast<char*>(&len), 4);
    frame += payload;
    ssize_t r = writen(fd, frame.data(), frame.size());
    // 处理 r...
}

// 解码核心（配合 Buffer 的典型 muduo 写法）
void onMessage(Buffer* buf) {
    while (buf->readableBytes() >= 4) {                 // 头部够了吗
        const char* p = buf->peek();
        uint32_t len;
        memcpy(&len, p, 4);
        len = ntohl(len);
        if (len > kMaxFrameLen) { /* 协议错误，断开连接 */ return; }
        if (buf->readableBytes() < 4 + len) break;      // payload 还没到齐
        buf->retrieve(4);                               // 跳过头部
        std::string payload = buf->retrieveAsString(len);
        // dispatch(payload);
    }   // while：一次可能凑出多帧（这就是"粘包"被正确处理的现场）
}
```

<a id="pnp-02-kernel"></a>

## 内核视角

- 每个连接有 **发送缓冲（`sk_sndbuf`）和接收缓冲（`sk_rcvbuf`）**，`ss -tm` 可看到实时占用。`write` 只是拷进发送缓冲 + 唤醒内核发送任务，**返回 ≠ 离开主机**
- 接收路径：网卡 → 软中断 → `tcp_v4_rcv` 按 **四元组** 找到 `tcp_sock` → 数据挂到 `sk_receive_queue`（`sk_buff` 链）→ 唤醒等在 `sk_wq` 上的进程。`read` 就是把 sk_buff 里的数据拷给用户——它看到的当然是 **连续字节**，分段信息（MSS 边界）在这一层早已抹掉
- **Nagle**：小包（小于 MSS）未确认时先攒着，攒到 MSS 或收到 ACK 才发。交互式/低延迟协议必须 `TCP_NODELAY`（muduo 默认开启）——它同时是"粘"的制造者和延迟杀手，见 [05 TTCP](./05_TTCP.md) 和 [12 TCP/IP](../../11-tcpip-protocols/)

<a id="pnp-02-pitfalls"></a>

## 坑点

- 以为「发 100 字节会一次收满 100」
- 缓冲合并：Nagle、接收窗口、应用读太慢
- 解决：**协议设计**，不是指望 TCP 帮你分包
- 用 `read` 返回值当"消息"，而不是当"字节片段"
- 长度字段不校验上限 → OOM / 越界
- 分隔符方案忘记转义：payload 里出现分隔符立刻解析错乱
- `write` 不检查返回值：部分写之后数据悄悄丢了

<a id="pnp-02-hft"></a>

## HFT 关联

| 场景 | 关系 |
|------|------|
| 行情/订单帧 | 交易所协议基本全是 **定长二进制帧**（长度前缀的特例：固定布局，见 [09 序列化](./09_Serialization.md)），解码零拷贝 |
| `TCP_NODELAY` | 撮合回报等小消息，Nagle 引入最多 40ms 级延迟——必关 |
| Buffer 扩容 | 峰值行情突发时解码缓冲反复 realloc 会引入抖动，muduo 式预留 + 上限校验是标准做法 |
| 抓包验证 | "为什么一次 send 被拆成两个包/两个 send 合成一个包"——[12.5 Wireshark](../../11.5-wireshark-packet-analysis/) 实验之一 |

<a id="pnp-02-quiz"></a>

## 自测题

1. 写端循环 `send("A",1)` 一万次，读端第一次 `read(buf, 4096)` 可能读到多少字节？这个数字由什么决定？
2. 为什么 muduo Buffer 前面要留 prependable bytes？举一个"在数据前面补内容"的实际场景。
3. 长度前缀解码器为什么要校验长度上限？攻击面是什么？
4. `readn` 循环里 `EINTR` 为什么必须 continue 而不能报错？
5. 关闭 Nagle 后吞吐会一定变差吗？什么场景反而更好？

<a id="pnp-02-refs"></a>

## 交叉引用

- 上一篇：[01 Socket 基础](./01_SocketBasics.md) · 下一篇：[03 自连接](./03_SelfConnect.md)
- [03.5 UNP Ch3 readn/writen](../../03.5-unix-network-api/) · [12 TCP/IP 协议](../../11-tcpip-protocols/) · [13 内核网络](../../12-kernel-networking/)
