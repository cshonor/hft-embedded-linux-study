# 18 — SO_REUSEPORT：多进程负载均衡

> **对应 Rosen:** 无
> **内核版本:** SO_REUSEPORT 3.9+；eBPF attach 4.6+

## 传统多进程监听同一端口

- `SO_REUSEADDR`：多个 socket 绑定同一地址，但只有最后一个能 accept
- 需要一个 master 进程 accept 后分发给 worker
- master 成为瓶颈

## SO_REUSEPORT

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
listen(sockfd, backlog);
```

- 多个进程/线程各自创建 socket，都绑定同一地址+端口
- 内核将连接**均匀分发**到各 socket
- 每个进程独立 accept，无 master 瓶颈

## 分发算法

| 版本 | 算法 | 特点 |
|------|------|------|
| 3.9-4.5 | hash(4-tuple) % N | 同一连接总是到同一 socket |
| 4.6+ | eBPF 自定义 | 可按任意逻辑分发（CPU 亲和性等） |

## eBPF 分发（4.6+）

```c
SEC("sk_reuseport")
int select_socket(struct sk_reuseport_md *ctx) {
    // 按 CPU 亲和性选择 socket
    int cpu = bpf_get_smp_processor_id();
    return cpu % NUM_SOCKETS;
}
```

## HFT 关联

| 场景 | SO_REUSEPORT 用途 |
|------|-------------------|
| 行情组播接收 | 多进程各自绑定同一组播组，各自独立处理 |
| TCP 行情连接 | 多 worker 线程各自 accept，避免 master 瓶颈 |
| CPU 亲和性 | eBPF 按当前 CPU 选择 socket，数据和处理在同核 |
