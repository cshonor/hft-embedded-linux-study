# 08 — XDP vs DPDK：内核旁路两条路

> **对应 Rosen:** 无
> **内核版本:** XDP 4.8+；DPDK 用户态

## 两条旁路路线

| 维度 | XDP（内核态旁路） | DPDK（用户态旁路） |
|------|------------------|-------------------|
| **核心理念** | 在内核最早点处理，跳过协议栈 | 完全绕过内核，用户态驱动 |
| **运行位置** | 内核态（驱动层） | 用户态 |
| **网卡控制** | 内核驱动管理，XDP 挂载 BPF 程序 | 用户态驱动（UIO/VFIO），内核放弃控制 |
| **内存管理** | page_pool（内核管理） | hugepage（用户态管理） |
| **零拷贝** | AF_XDP zero-copy（page_pool 映射） | RTE buffer + hugepage |
| **CPU 占用** | NAPI 轮询 + BPF（可配置） | 100% 轮询（DPDK polling mode） |
| **中断** | 可关闭（busy poll） | 完全关闭 |
| **协议栈共存** | 是（非 XDP 路径仍走协议栈） | 否（网卡被 DPDK 独占） |
| **部署复杂度** | 低（加载 BPF 程序） | 高（绑定驱动 + hugepage + NUMA） |
| **调试工具** | 内核工具（bpftool/xdp-tools） | DPDK 工具（testpmd/dpdk-proc-info） |
| **生态** | 内核主线，跟随内核更新 | 独立项目，需适配内核版本 |

## 性能对比（典型值）

| 指标 | XDP | DPDK | 差距 |
|------|-----|------|------|
| 收包延迟 | ~100-200 ns | ~50-100 ns | DPDK 快 2x |
| 包处理速率 | ~24 Mpps/core | ~40+ Mpps/core | DPDK 快 1.5-2x |
| CPU 占用 | 可配置 | 100% | XDP 灵活 |
| 额外内存 | page_pool（少量） | hugepage（GB 级） | XDP 少 |

## HFT 选型决策

```
需要极致延迟（< 1μs）？
  ├─ 是 → co-location 环境 → DPDK
  └─ 否 → 需要内核功能（路由/TCP/安全）？
            ├─ 是 → XDP + AF_XDP
            └─ 否 → 延迟要求 < 5μs？
                      ├─ 是 → DPDK（非 co-location 也可）
                      └─ 否 → XDP（性价比最高）
```

## 混合方案

部分 HFT 系统使用混合架构：
- **行情接收**：DPDK（超低延迟，网卡独占）
- **管理通道**：内核协议栈（SSH/监控/控制）
- **行情分发**：XDP CPUMAP（在内核层分发到非 DPDK 网卡）
- 需要双网卡：一张 DPDK 独占，一张内核管理

## 常见误区

| 误区 | 事实 |
|------|------|
| XDP 一定能替代 DPDK | 不一定，co-location 场景 DPDK 延迟优势明显 |
| DPDK 一定比 XDP 快 | 非零拷贝 AF_XDP 比 DPDK 慢，但 zero-copy 差距缩小 |
| XDP 不需要 CPU | Native XDP 仍需要 CPU 轮询（NAPI 驱动） |
| DPDK 不需要内核 | DPDK 需要 UIO/VFIO 内核模块，只是数据路径旁路 |
