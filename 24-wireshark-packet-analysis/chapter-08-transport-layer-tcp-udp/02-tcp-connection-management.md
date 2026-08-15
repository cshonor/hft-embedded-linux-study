# 8.1.3–8.1.5 TCP 连接管理（握手 · 断开 · 重置）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：三次握手建连、四次挥手断连、RST 拒绝/异常终止——Wireshark 中按标志位识别。

## 核心知识点

### 握手目的

1. 确认对端在线、端口在监听  
2. 交换 **ISN（初始序列号）**  
3. 协商选项（如 **MSS**）

---

### 8.1.3 三次握手

| 步 | 方向 | 标志 | 要点 |
|----|------|------|------|
| 1 | A → B | **SYN** | A 的 ISN；可带 MSS 等选项 |
| 2 | B → A | **SYN + ACK** | B 的 ISN；Ack = A.ISN + 1 |
| 3 | A → B | **ACK** | Ack = B.ISN + 1；此后可传数据 |

```text
A ── SYN ──────────────► B
A ◄── SYN,ACK ───────── B
A ── ACK ──────────────► B   → ESTABLISHED
```

**Wireshark 过滤器**

| 场景 | 表达式 |
|------|--------|
| 纯 SYN（第一步） | `tcp.flags.syn==1 && tcp.flags.ack==0` |
| 任意握手包 | `tcp.flags.syn==1` |
| 某连接 | 加上 `ip.addr==x` 与 `tcp.port==443` |

**专家信息**：正常握手多为 Chat；失败常见 **RST** 或超时重传。

---

### 8.1.4 连接断开（四次挥手）

| 步 | 方向 | 标志 | 说明 |
|----|------|------|------|
| 1 | A → B | **FIN, ACK** | A 数据发完，请求关闭 |
| 2 | B → A | **ACK** | 确认 A 的 FIN |
| 3 | B → A | **FIN, ACK** | B 也发完，请求关闭 |
| 4 | A → B | **ACK** | 最终确认 |

> 现代栈常把 FIN 与 ACK 同包；抓包可能看到 **3 个包** 的变体，但逻辑仍是「双向各关一半」。

**过滤器**：`tcp.flags.fin==1`

**TIME_WAIT**：主动关闭方在末 ACK 后等待 2MSL（抓包上可能仍见零星重传 ACK）。

---

### 8.1.5 TCP 重置（RST）

| 项 | 说明 |
|----|------|
| **RST** | 连接**异常中止**或**拒绝连接** |
| 场景 | 端口未监听、防火墙拒绝、扫描、中间设备切断 |
| 案例 | 访问路由器 **80** 但未开 Web → **RST,ACK**，无后续 HTTP |

**Wireshark**：`tcp.flags.reset==1`

| 与 FIN 区别 | RST 通常**立即**丢弃连接状态；FIN 为优雅关闭 |
| 捕获 BPF | `tcp&4==4`（见 [§4.5](../chapter-04-capture-packet/05-filter-basics.md)） |

## 抓包/实操记录

| 实验 | 预期 |
|------|------|
| 正常 Web | `tcp.stream` 内 SYN → SYN,ACK → ACK，末尾 FIN 序列 |
| 拒绝连接 | `nc` 访问关闭端口 → 单包或少量 **RST** |
| Follow Stream | 右键 → Follow TCP Stream 看握手后明文（HTTP） |
| 统计 | `tcp.flags.syn==1` 计数是否异常多（扫描） |

```bash
tshark -r cap.pcapng -Y "tcp.flags.syn==1 && tcp.flags.ack==0" -T fields -e ip.src -e tcp.dstport
```

## 疑问与总结

- **半开连接**：只收到 SYN 无 SYN,ACK → 对端未监听或防火墙丢包。
- 重传、零窗口、乱序 → [§03](./03-tcp-reliability-flow-control.md) 与 [第5章 Expert](../chapter-05-advanced-feature/08-expert-info.md)。
