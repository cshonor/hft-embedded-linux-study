# 01 — NAPI 现代化：threaded NAPI 与 busy polling

> **对应 Rosen:** Ch1（NAPI 基础）+ Ch14（高级主题 RPS/RFS）
> **内核版本:** NAPI 原始设计 2.5+；threaded NAPI 5.11+；SO_BUSY_POLL 3.11+

## NAPI 基础回顾

NAPI（New API）是 Linux 网卡收包的核心机制：
- 中断驱动 → 轮询模式切换：首个包触发中断后关闭中断，进入轮询
- `struct napi_struct`：每个网卡注册一个 NAPI 实例
- `napi_poll()` 回调：驱动提供的轮询函数，每次调用处理 budget 个包
- 轮询完成后重新开中断，等待下一批

## 现代变化

### Threaded NAPI（5.11+）

传统 NAPI 在软中断上下文（`NET_RX_SOFTIRQ`）执行：
- 软中断与其它 ksoftirqd 共享 CPU，可能被抢占
- 无法绑定到特定 CPU 核心

Threaded NAPI 将轮询移到独立内核线程：
```
# 启用 threaded NAPI
echo 1 > /sys/class/net/eth0/threaded
```
- 每个 NAPI 实例一个内核线程（`napi/eth0`）
- 可通过 `chrt` / `taskset` 设置优先级和 CPU 亲和性
- 代价：线程切换开销，但隔离性更好

### Busy Polling（SO_BUSY_POLL，3.11+）

传统流程：数据到达 → 中断 → 软中断 → socket 可读 → 唤醒用户进程

Busy polling 让用户进程主动轮询 NAPI，跳过中断：
```c
int val = 50;  // busy poll 时间（微秒）
setsockopt(sockfd, SOL_SOCKET, SO_BUSY_POLL, &val, sizeof(val));
```
- `recvmsg()` 时直接调用 `napi_poll()` 检查是否有数据
- 以 CPU 100% 换取低延迟（不等待中断）
- HFT 场景常用：交易进程独占一个 CPU 核心，持续 busy poll

### NAPI budget 变化

- 默认 budget = 64（每次轮询最多处理 64 个包）
- 现代驱动可调整：`ethtool -C eth0 rx-usecs N`
- 高吞吐场景增大 budget，低延迟场景减小并配合 busy polling

## HFT 关联

| 特性 | HFT 用途 |
|------|---------|
| SO_BUSY_POLL | 行情接收进程持续轮询，跳过中断唤醒延迟 |
| Threaded NAPI | NAPI 线程绑定到隔离 CPU，避免软中断争抢 |
| budget 调优 | 小 budget + busy poll = 最低收包延迟 |

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| NAPI 上下文 | 仅软中断 | 软中断 或 threaded NAPI |
| busy polling | 不存在 | SO_BUSY_POLL + NAPI_ID |
| budget 控制 | 固定 64 | 可调，配合 ethtool coalescing |
