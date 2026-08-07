# 16 — MSG_ZEROCOPY：零拷贝发送

> **对应 Rosen:** Ch11（sendmsg 只有拷贝模式）
> **内核版本:** MSG_ZEROCOPY 4.14+

## 传统发送路径

```
用户态 buffer → copy_from_user() → 内核 skb data → 驱动 → NIC DMA
```
- sendmsg() 将用户态数据拷贝到内核 sk_buff
- 大数据量时拷贝开销显著（memcpy 占 CPU）

## MSG_ZEROCOPY 机制

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt));

// 发送时标记零拷贝
sendmsg(sockfd, &msg, MSG_ZEROCOPY);
```

工作原理：
1. 内核不拷贝用户态数据，而是将用户态 page 映射到 sk_buff
2. NIC DMA 直接从用户态 page 读取数据
3. 发送完成后内核通知用户态（通过 errqueue）
4. 用户态收到通知后才能修改/释放 buffer

## 通知机制

零拷贝发送是异步的，内核通过 `MSG_ERRQUEUE` 通知完成：
```c
struct msghdr msg = {};
char control[CMSG_SPACE(sizeof(struct sock_extended_err))];
msg.msg_control = control;
msg.msg_controllen = sizeof(control);
recvmsg(sockfd, &msg, MSG_ERRQUEUE);
// 检查完成通知，可以安全释放 buffer
```

## 性能提升

| 数据量 | 传统 sendmsg | MSG_ZEROCOPY | 提升 |
|--------|-------------|-------------|------|
| 1 KB | ~1 μs | ~1.5 μs | 更慢（通知开销） |
| 64 KB | ~5 μs | ~1.5 μs | 3x |
| 1 MB | ~80 μs | ~3 μs | 25x |

> 小包零拷贝反而更慢（通知开销 > 拷贝开销），HFT 交易报文通常很小，不一定适合。

## HFT 关联

| 场景 | MSG_ZEROCOPY 适用性 |
|------|---------------------|
| 交易报文（< 1KB） | 不推荐（通知开销 > 拷贝开销） |
| 行情转发（大包） | 推荐（减少大包拷贝） |
| 批量发送 | 配合 io_uring SEND_ZC 效果更好 |
