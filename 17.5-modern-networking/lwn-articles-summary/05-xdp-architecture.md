# 05 — XDP 架构与 use case 全景

> **对应 Rosen:** 无（书出版时 XDP 不存在）
> **内核版本:** XDP hook 4.8+；AF_XDP 4.18+；XDP multi-attach 5.1+

## XDP 是什么

XDP（eXpress Data Path）是内核网络栈的**最早数据路径 hook**：
- 在驱动层、sk_buff 分配之前执行
- 以 eBPF 程序形式运行（JIT 编译为原生指令）
- 可以查看/修改包内容、丢弃、重定向

## 四种 XDP 模式

| 模式 | 挂载点 | 硬件要求 | 性能 |
|------|--------|---------|------|
| Native XDP | 驱动层（网卡驱动支持） | 驱动需实现 XDP hook | 最高 |
| Offloaded XDP | 网卡硬件（SmartNIC） | 网卡支持 eBPF 卸载 | 极致（不占 CPU） |
| Generic XDP | 协议栈入口（sk_buff 之后） | 无 | 最低（仍分配 sk_buff） |
| SKB-mode XDP | 类似 Generic | 无 | 低 |

## XDP 程序类型

```c
// 最简 XDP 程序：丢弃所有包
SEC("xdp")
int xdp_drop_all(struct xdp_md *ctx) {
    return XDP_DROP;
}

// 检查目标端口，丢弃非行情端口
SEC("xdp")
int xdp_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    struct ethhdr *eth = data;
    if ((void*)(eth+1) > data_end) return XDP_DROP;
    if (eth->h_proto != htons(ETH_P_IP)) return XDP_PASS;
    // ... 检查 IP/TCP/UDP 头
    return XDP_PASS;
}
```

## XDP use case 全景

| 场景 | XDP 动作 | HFT 关联 |
|------|---------|---------|
| DDoS 防护 | XDP_DROP 丢弃攻击包 | 保护交易服务器 |
| 负载均衡 | XDP_REDIRECT 到后端 | 行情分发 |
| 包过滤 | XDP_DROP 无关包 | 行情流早过滤 |
| 协议预处理 | XDP_TX 响应 | ARP/ICMP 快速响应 |
| 监控统计 | XDP_PASS + 计数 | 延迟/丢包监控 |
| AF_XDP | XDP_REDIRECT 到用户态 | 内核旁路收包 |

## HFT 关联

XDP 是 HFT 在**不使用 DPDK 时的最佳内核态方案**：
- 行情组播流：XDP 过滤无关组播组，只放行目标行情
- 延迟监控：XDP 打时间戳，测量 NIC → 内核 → 用户态各段延迟
- AF_XDP：零拷贝将行情包送到用户态，性能接近 DPDK

## 与 DPDK 的定位区别

| 维度 | XDP | DPDK |
|------|-----|------|
| 运行位置 | 内核态 | 用户态 |
| 内核参与 | 是（但极早） | 否（完全旁路） |
| 部署复杂度 | 低（加载 BPF 程序） | 高（绑定 UIO/VFIO 驱动） |
| 灵活性 | 高（可与其他内核功能共存） | 低（网卡被独占） |
| 适用场景 | 中低频、需内核功能 | 超低延迟、co-location |
