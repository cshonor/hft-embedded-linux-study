# Ch 7 §6 NUD 状态机 · Network Unreachability Detection

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 6. 网络不可达检测（NUD）状态机

内核用 **NUD（Network Unreachability Detection）** 状态机维护 **每条 neighbour 是否仍可达** — 驱动 **探测、过期、失败** 与 **队列释放**。

---

## 主要状态

| 状态 | 含义 |
|------|------|
| **`NUD_NONE`** | 刚创建，尚无 L2 信息 |
| **`NUD_INCOMPLETE`** | **解析中** — 已发 ARP/NS，**等响应**；包在 **arp_queue** |
| **`NUD_REACHABLE`** | **确认可达** — 近期有 **双向确认** |
| **`NUD_STALE`** | 缓存 **过期** — 仍可用发几包，**背景 probe** |
| **`NUD_DELAY`** | 发后 **短暂等待** 再 probe |
| **`NUD_PROBE`** | **主动 unicast NS/ARP** 验证 |
| **`NUD_FAILED`** | **解析/探测失败** — 不可达，队列 **drop** |

```
         solicit
NONE → INCOMPLETE ──reply──→ REACHABLE
                │                │
                fail             │ timeout / no confirm
                ↓                ↓
             FAILED            STALE → DELAY/PROBE → REACHABLE or FAILED
```

---

## 行为影响

| 状态 | 发包 |
|------|------|
| **REACHABLE / STALE** | **直接 xmit**（STALE 可能触发 **异步确认**） |
| **INCOMPLETE** | **排队** — 首包 **延迟** |
| **FAILED** | **丢弃** 并向上 **报错**（EHOSTUNREACH 等） |

**HFT 尖刺：** 交换机 **MAC 漂移**、**换端口** → 大量 **STALE/INCOMPLETE** → **毫秒级抖动** — **静态 permanent** 或 **快速 fail 告警**。

---

## 定时器与 sysctl

- **`base_reachable_time`** — REACHABLE 维持时间  
- **`delay_first_probe_time`** — STALE 后首次 probe  
- **`ucast_solicit` / `mcast_solicit`** — 重试次数  

```bash
sysctl net.ipv4.neigh.default.base_reachable_time_ms
sysctl net.ipv6.neigh.default.base_reachable_time_ms
```

---

## `nud_state` 与用户态

**`ip neigh show`** 第三列即 **状态**（`REACHABLE`、`STALE`、`FAILED`…）— §7。

---

← [5. DAD](./section-5-重复地址检测-DAD.md) · [Ch 7](../README.md) · 下一节 [7. 用户空间](./section-7-用户空间交互.md)
