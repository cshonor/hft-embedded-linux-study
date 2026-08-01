# Ch 5 §4 FIB 下一跳异常 · FIB Nexthop Exceptions

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 4. FIB 下一跳异常 (FIB Nexthop Exceptions)

**Linux 3.6+** 引入 — 处理 **非用户空间配置** 导致的 **动态 nexthop 变更**（ICMP 重定向、**PMTUD** 等），而不必 **重写整张 FIB 表**。

---

## 动机

| 事件 | 需要记录什么 |
|------|--------------|
| **ICMP Redirect** | 「这目的 **别经我**，走 **更优 GW**」 |
| **PMTU 发现** | 到某目的 **有效 MTU 变小** — 影响 **df bit 发包** |
| 临时路径变化 | **per-destination** 覆盖 **静态 fib_nh** |

用户 **未执行 `ip route`**，但 **数据平面** 需 **短期/长期** 调整 — **exception 表** 挂接在 **dst / nexthop** 侧。

---

## 行为概念

```
fib_lookup 命中 static route
  → 查 nexthop exception 缓存
       ├─ 有 PMTU exception → 使用 **更小 MTU** 发包
       └─ 有 redirect 学习   → 可能 **更新 host route** 或 **短期 GW**
```

**与 §7 路由缓存移除的关系：** 3.6 去掉 **per-flow 路由缓存** 后，**异常机制** 部分承担 **「细粒度 per-dest 状态」** — 但 **不是** 恢复旧 routing cache 的 DoS 面。

---

## HFT 要点

| 现象 | 可能原因 |
|------|----------|
| **TCP 突然改 MSS** | **PMTU exception** + ICMP Fragmentation Needed |
| **短流走「奇怪」GW** | 曾收 **ICMP redirect**（共置 L2 常 **应关 redirect 接受**） |
| **排查** | `ip route get <dst>`、`tracepath`、`nstat` |

**共置建议：** `net.ipv4.conf.all.accept_redirects=0`（及 **secure_redirects**）— 防 **恶意/次优 redirect** 干扰 **固定路径**。

→ ICMP 重定向详 §6 · PMTU：[Ch 3 §1](../../chapter-03-icmp/notes/section-1-ICMPv4的实现与消息流转.md) · [Ch 4 §6](../../chapter-04-ipv4/notes/section-6-分片与重组.md)

---

← [3. 数据结构](./section-3-核心数据结构-FIB表与Info与Alias.md) · [Ch 5](../README.md) · 下一节 [5. 策略路由](./section-5-策略路由.md)
