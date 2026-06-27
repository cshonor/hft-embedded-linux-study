# Ch 4 §4 IP 选项 · IP Options

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 4. IP 选项 (IP Options)

IPv4 头 **options 区最多 40 字节**（`ihl` 最大 15 → 60B 头 − 20B 固定 = 40B options）。现代互联网 **极少见** — 但 **Record Route / Timestamp** 仍出现在 **traceroute 类工具** 与 **安全审计** 讨论中。

---

## 内核表示

**`struct ip_options`** — 编译后的选项状态（偏移、指针、flags）：

| API | 作用 |
|-----|------|
| **`ip_options_compile()`** | 解析 **入站** IP 头中的 options → 填充 `ip_options` |
| **`ip_options_build()`** | **出站** 时把 options 写入 IP 头 |
| **`ip_options_echo()`** | ICMP 差错时 **回显** 选项（RFC 要求部分类型） |

---

## 常见选项类型

| Option | 用途 |
|--------|------|
| **Record Route (RR)** | 沿途路由器写入 **经过的 IP** — 早期 traceroute 思路 |
| **Timestamp (TS)** | 记录 **时间戳 + 可选地址** |
| **LSRR / SSRR** | 源路由（**现代通常禁用/忽略** — 安全） |
| **End of Option List** | 填充对齐 |

**Timestamp 结构（概念）：** 每个条目 **type + length + pointer + (address, timestamp)* ** — 指针指示 **下一个空槽**。

**Record Route：** 每经一跳路由器，若还有空间则 **写入 outgoing 接口 IP**。

---

## 安全与 HFT

| 点 | 说明 |
|----|------|
| **源路由** | 历史上 **绕过防火墙** — 内核/网络常 **no ip source-route** |
| **处理成本** | 选项解析 **慢路径** — 畸形 option → **`InAddrErrors`** |
| **HFT 生产** | 行情/发单 **无 IP options** — 见到带 options 包多为 **扫描/攻击** |

**现代 traceroute** 多用 **UDP/TCP/ICMP probe + TTL**，而非 RR 选项。

---

← [3. 多播接收](./section-3-接收IPv4多播数据包.md) · [Ch 4](../README.md) · 下一节 [5. 发送路径](./section-5-发送IPv4数据包.md)
