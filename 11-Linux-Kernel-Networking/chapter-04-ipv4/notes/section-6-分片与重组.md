# Ch 4 §6 分片与重组 · Fragmentation & Defragmentation

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 6. 分片与重组 (Fragmentation & Defragmentation)

当 IP 包 **大于出接口 MTU** 且 **未设 DF** 时，必须 **分片 (fragmentation)**；接收端 **defrag** 还原。HFT 环境通常 **避免分片** — 但排查 **PMTU/丢包** 必须懂机制。

---

## 分片 — `ip_fragment()`

触发：`ip_finish_output` 发现 **skb->len > mtu** 且 **!DF**。

```
原始 skb
  → ip_fragment() 按 MTU 切多片
  → 每片：独立 IP 头（同 id，不同 offset/MF）
  → 各片独立 xmit
```

| 路径 | 说明 |
|------|------|
| **快速路径** | 预分配 skb 布局、已知头长、无复杂选项 |
| **慢速路径** | 大选项、罕见边界、内存压力 |

**DF=1 且超长：** 不发片 — **`icmp_send(Fragmentation Needed, mtu)`** → PMTUD（[Ch 3 §1](../../chapter-03-icmp/notes/section-1-ICMPv4的实现与消息流转.md)）。

---

## 重组 — `ip_defrag()` / `ip_frag_reasm()`

```
各分片到达 ip_defrag()
  → 按 (saddr,daddr,protocol,id) 哈希到 ipq 队列
  → 收齐所有 offset 连续片 + 末片 MF=0
  → ip_frag_reasm() 拼成完整 skb
  → 交 ip_local_deliver_continue → L4
```

**`struct ipq`：** 管理 **同一 id** 的片链表；**超时** 丢弃未完成组 — **`timeout` sysctl** 防 **分片耗尽内存** 攻击。

| 风险 | 缓解 |
|------|------|
| **Teardrop 类攻击** | 重叠/乱序片校验 |
| **内存占用** | `ip_frag_mem_limit`、早期 drop |
| **CPU** | 重组 **远贵于** 整包 — HFT **禁分片** |

---

## HFT 实践

```
推荐：应用报文 + IP/UDP 头 < Path MTU（常 1500 或 jumbo 9000）
TCP：  MSS 协商 + DF
UDP：  固定 payload 上限；跨网段确认 **中间 MTU**
防火墙：勿 block **ICMP type 3 code 4**
```

**DPDK：** 分片可在 **用户态** 或 **不支持** — 与内核路径分开规划。

---

← [5. 发送路径](./section-5-发送IPv4数据包.md) · [Ch 4](../README.md) · 下一节 [7. 转发](./section-7-数据包转发.md)
