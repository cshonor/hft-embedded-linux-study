# 6.6 应用过滤器

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · GUI：[§4.5](../chapter-04-capture-packet/05-filter-basics.md) · 速查：[cheatsheet/notes.md](../cheatsheet/notes.md)

**核心主旨**：TShark 同时支持 **BPF 捕获滤** 与 **显示滤**；tcpdump 仅 BPF。

## 核心知识点

### TShark 双重体系

| 类型 | 参数 | 语法 | 阶段 |
|------|------|------|------|
| **捕获过滤器** | **`-f "..."`** | **BPF**（tcpdump 同） | 抓包时丢弃 |
| **显示过滤器** | **`-Y "..."`** | **Wireshark 字段** | 仅影响输出/统计 |

```bash
# 只抓 80
tshark -ni 1 -w out.pcapng -f "tcp port 80"

# 离线只看目标端口 80
tshark -r out.pcapng -Y "tcp.dstport == 80"

# 组合：抓宽、滤窄
tshark -r out.pcapng -Y "http && ip.addr==192.168.1.10" -T fields -e frame.number -e http.request.uri
```

> **引号**：Windows/Linux 均建议用双引号包裹表达式；内含空格时必须引号。

### tcpdump BPF

```bash
sudo tcpdump -nni eth0 'tcp dst port 80' -w http.pcap
sudo tcpdump -nni eth0 host 10.0.0.1 and port 443
```

| 高级 | 说明 |
|------|------|
| **`-F <file>`** | 从文件加载 BPF 规则 |
| **易错** | 规则文件内**禁止写注释** `#`，否则解析失败 |

### 记忆口诀

```text
TShark:  -f = BPF 抓    -Y = 显示滤 看
tcpdump: 末尾引号 BPF，无 -Y
```

## 抓包/实操记录

| 练习 | 命令 |
|------|------|
| 瘦身大文件 | `tshark -r big.pcapng -Y "tcp.port==443" -w small.pcapng` |
| RST 捕获 | tcpdump `'tcp[13] & 4 != 0'` 或 `tcp&4==4` |
| 统计+滤 | `tshark -r f.pcapng -Y "dns" -q -z io,phs` |

## 疑问与总结

- 能 `-Y` 离线滤就不应用 `-f` 抓太窄（除非流量极大）。
- `-Y` 不改变原文件；写新文件需配合 **`-w`** 导出（部分版本用管道或 editcap）。
