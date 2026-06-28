# Ch 5 §5 策略路由 · Policy Routing

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 5. 策略路由 (Policy Routing)

自 **Linux 2.4** 起 — 路由决策 **不限于目的 IP**，还可按 **源地址、TOS、mark、uid** 等选 **不同路由表**（最多 **255** 张自定义表 + local/main/default）。

---

## 规则 + 多表

```bash
# 查看策略规则（优先级从小到大匹配）
ip rule show

# 典型：from 10.1.1.0/24 lookup 100
ip rule add from 10.1.1.0/24 table 100
ip route add default via 192.168.2.1 table 100
```

| 组件 | 作用 |
|------|------|
| **`ip rule`** | **policy** — 匹配 flow → **指定 table ID** |
| **`ip route … table N`** | 该表内 **独立 FIB** |
| **默认** | 无 rule 命中 → **table main (254)** |

**查找顺序：** `fib_rules_lookup()` → 命中 rule → **`fib_lookup` 在对应 table**。

---

## 匹配维度（常见）

| 键 | 场景 |
|----|------|
| **from / to** | 源/目的前缀分流 |
| **tos / dscp** | QoS 不同出口 |
| **fwmark** | **iptables/nft MARK** → 不同 ISP/VPN |
| **iif / oif** | 入接口/出接口策略 |
| **uidrange** | 按进程用户选路（Android/容器常见） |

---

## 与 HFT

| 用法 | 说明 |
|------|------|
| **行情 vs 发单分离** | 不同 **源 IP / mark** → 不同 **table/default GW** |
| **管理网 vs 交易网** | **from 管理段** 走 mgmt 表，**交易 daemon** 绑 **交易源地址** |
| **复杂度成本** | 每包 **rule 链 + 二次 fib_lookup** — 规则 **宜少宜静态** |
| **DPDK** | 旁路栈；策略在 **用户态** 或 **静态 ARP/ND** 等价实现 |

→ 高级场景：[Ch 6 高级路由](../../chapter-06-高级路由.md) · Netfilter mark：[Ch 9](../../chapter-09-Netfilter.md)

---

← [4. 下一跳异常](./section-4-FIB下一跳异常.md) · [Ch 5](../README.md) · 下一节 [6. ICMP 重定向](./section-6-ICMPv4重定向消息.md)
