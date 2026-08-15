# HFT 场景 02：NIC Offload 对抓包的影响

> [总览](./00-overview.md) · [基础：ch04 抓包](../chapter-04-capture-packet/chapter-summary.md) · [HFT 场景 01：TCP 延迟](./01-tcp-latency-analysis.md)

**核心问题**：现代网卡（NIC）的 TSO/GRO/GSO 等 offload 功能会把多个包合并成一个大包交给内核。Wireshark 抓到的是**合并后的包**，不是真实线缆上的包。在 HFT 场景中，这会严重误导延迟分析。

## 1. Offload 类型

| Offload | 全称 | 方向 | 作用 | 对 Wireshark 的影响 |
|---------|------|------|------|-------------------|
| **TSO** | TCP Segmentation Offload | TX | 内核给网卡一个大段，网卡分拆成多个 MSS | Wireshark 看到一个巨大的 TCP 段（> MSS） |
| **GRO** | Generic Receive Offload | RX | 网卡合并同流的多个小包为一个大包 | Wireshark 看到一个大包，丢失原始包边界 |
| **GSO** | Generic Segmentation Offload | TX | 类似 TSO 但由软件分拆 | 同 TSO |
| **LRO** | Large Receive Offload | RX | 硬件版 GRO（旧技术） | 同 GRO，更激进 |
| **Checksum Offload** | TX/RX | 网卡计算校验和 | Wireshark 显示 checksum=0x0000（TX 方向，正常现象） |

## 2. 问题示例

### TSO 导致的假象

```
真实线缆：  [Seq=1000, len=1448] [Seq=2448, len=1448] [Seq=3896, len=1448]
                                    ↓ TSO 合并
Wireshark： [Seq=1000, len=4344]  ← 一个巨大的包！
```

**后果**：
- 你以为发了 1 个包，实际线缆上是 3 个包
- 包的 Timestamp 是合并时刻，不是真实发送时刻
- RTT 计算可能不准

### GRO 导致的假象

```
真实线缆：  [包1 t=10μs] [包2 t=20μs] [包3 t=30μs]
                              ↓ GRO 合并
Wireshark： [合并包 t=30μs]  ← 丢失了包1和包2的时间戳！
```

**后果**：
- HFT 延迟分析中，微秒级时间戳被抹掉
- 无法看到真实的包到达间隔
- 误判延迟来源

## 3. 检查 Offload 状态

```bash
# 查看网卡 offload 配置
ethtool -k eth0

# 关键输出
# tcp-segmentation-offload: on    ← TSO
# generic-receive-offload: on     ← GRO
# generic-segmentation-offload: on ← GSO
# rx-checksumming: on             ← RX Checksum Offload
```

## 4. 抓包前关闭 Offload

```bash
# 关闭 TSO/GRO/GSO（需要 root）
sudo ethtool -K eth0 tso off gro off gso off

# 验证
ethtool -k eth0 | grep -E "tcp-segmentation|generic-receive|generic-segmentation"

# 抓包
sudo tcpdump -nni eth0 -w trade_offload_disabled.pcapng -s 0

# 抓完恢复（生产环境谨慎！关闭 offload 会增加 CPU 开销和延迟）
sudo ethtool -K eth0 tso on gro on gso on
```

| 环境 | 建议 |
|------|------|
| **生产抓包** | 临时关闭 GRO（影响最小），TSO 可留（TX 方向不影响 RX 分析） |
| **实验环境** | 全关，看到真实线缆包 |
| **HFT 生产** | 通常已关闭 GRO（为低延迟），但 TSO 可能保留 |

## 5. Checksum Offload 误判

Wireshark 常显示 TX 方向包的 TCP checksum 为 `0x0000`，这不是错误——网卡会在发送时计算真实校验和。

| 现象 | 原因 | 处理 |
|------|------|------|
| `[Bad checksum]` | RX checksum offload 计算 + Wireshark 验证 | 检查 `ethtool -K eth0 rx off` 是否被关闭 |
| Checksum = 0x0000 | TX 方向，网卡还没填 | 正常，忽略 |
| 大量 Bad checksum | 网卡硬件故障 | 更换网卡 |

## 6. HFT 自测题

1. 你在分析交易服务器的 pcap，发现一个 TCP 段的 len=64240（远大于 MSS 1448）。原因是什么？如何看到真实线缆包？
2. 关闭 GRO 后，Wireshark 中的包数量增加了 3 倍，但 RTT 分布没变。说明了什么？
3. 为什么 HFT 生产环境通常关闭 GRO 但保留 TSO？
4. 你在 pcap 中看到所有 TX 包的 checksum 都是 0x0000，这是 bug 吗？

## 交叉引用

- [基础：抓包文件](../chapter-04-capture-packet/01-use-capture-files.md)
- [基础：高级捕获选项](../chapter-04-capture-packet/04-advanced-capture-options.md)
- [HFT 场景 01：TCP 延迟](./01-tcp-latency-analysis.md)
- [HFT 场景 03：内核旁路](./03-kernel-bypass-limitations.md)
- [HFT 模块：内核网络](../../12-kernel-networking/)
