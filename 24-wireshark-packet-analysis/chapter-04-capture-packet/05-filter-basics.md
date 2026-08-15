# 4.5 过滤器基础

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 速查：[cheatsheet/notes.md](../cheatsheet/notes.md)

**核心主旨**：区分**捕获过滤器**与**显示过滤器**的机制与场景，掌握 BPF 与显示表达式语法。

## 核心知识点

### 4.5.1 捕获过滤器（Capture Filters）

| 项 | 说明 |
|----|------|
| **阶段** | **抓包时**生效；不满足的包**不进入**捕获文件（省 CPU、磁盘） |
| **语法** | **BPF**（Berkeley Packet Filter），与 tcpdump 相同 |
| **位置** | `Capture Options` → 输入框；或 `tcpdump -f '...'` |

**原语结构**：`[proto] [dir] type id`

| 限定词 | 示例 |
|--------|------|
| **Type** | `host`, `net`, `port` |
| **Dir** | `src`, `dst`（可省略表示双向） |
| **Proto** | `ip`, `tcp`, `udp`, `arp` |

```text
host 192.168.1.1 and port 443
net 10.0.0.0/24
tcp port 80
```

**协议域过滤器（字节级）**

- 语法：`proto[offset:length]`，配合 `&` 按位与。
- **案例：只抓 TCP RST**

```text
tcp[13] & 4 != 0
```

或教材写法：**`tcp&4==4`**（检查标志字节中 RST 位，RST = 4）

> 显示过滤器用 `tcp.flags.reset==1`；捕获用 BPF 字节语法，勿混用。

### 4.5.2 显示过滤器（Display Filters）

| 项 | 说明 |
|----|------|
| **阶段** | 对**已捕获**文件；只隐藏列表显示，**不删**文件中数据 |
| **语法** | Wireshark 自有字段名（见 `Analyze` → `Display Filter Expression`） |

**比较**：`==` `!=` `>` `<` `>=` `<=`

**逻辑**：`and` `or` `xor` `not`

| 案例 | 表达式 |
|------|--------|
| 隐藏 ARP 噪声 | `!arp` |
| 某 IP | `ip.addr == 192.168.1.1` |
| TCP RST（显示） | `tcp.flags.reset == 1` |
| 重传（进阶） | `tcp.analysis.retransmission` |

输入栏绿色 = 合法；红色 = 语法错误。

### 4.5.3 保存过滤器与工具栏

| 功能 | 操作 |
|------|------|
| 保存表达式 | 显示过滤器栏右侧 → 书签/保存；`Preferences` → **Filter Expressions** |
| 固定到工具栏 | 为保存项设 **Label** → 出现在 Packet List **上方按钮** |
| 场景 | 一键 `!arp`、一键 RST、一键 `dns` |

**效率**：复杂表达式写一次，排障时一键切换。

> **拓展**：TCP 排障可组合 `tcp.analysis` 系列（重传、乱序、零窗口）——见 [cheatsheet](../cheatsheet/notes.md) 与后续 TCP 章。

## 抓包/实操记录

| 对比实验 | 步骤 |
|----------|------|
| 捕获 vs 显示 | 捕获 `port 80` 抓包 → 再显示 `dns` → 列表为空但文件仍含 80 流量 |
| RST | 显示 `tcp.flags.reset==1`；捕获用 `tcp[13]&4!=0`（若驱动支持） |
| 工具栏 | 保存 `!arp` 为按钮，观察列表清爽度 |

## 疑问与总结

| | 捕获过滤器 | 显示过滤器 |
|---|------------|------------|
| 时机 | 抓之前 | 抓之后 |
| 语法 | BPF / tcpdump | Wireshark 字段 |
| 数据文件 | 真的没抓到 | 仍在，只是不显示 |
| 记错后果 | 永远缺包 | 可随时 `清除` 恢复 |

- **原则**：能显示滤就不捕获滤，避免「抓少了无法反悔」；长跑、高流量再捕获滤减负。
