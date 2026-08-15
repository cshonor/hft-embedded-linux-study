# 6.3 捕获和保存流量

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：选网卡、写 pcap、限制包数、读离线文件——TShark 与 tcpdump 通用骨架。

## 核心知识点

### 1. 指定网卡（Interface）

| 工具 | 做法 |
|------|------|
| **TShark** | `tshark -D` 列出**数字编号**列表；Windows 常为 GUID，用 **`-i 1`** 等编号，勿手写 GUID |
| **tcpdump** | `tcpdump -i eth0`（Linux 接口名明确） |

```bash
tshark -D
tshark -i 1 -w out.pcapng
sudo tcpdump -i eth0 -w out.pcap
```

### 2. 保存到文件（Write）

| 参数 | 说明 |
|------|------|
| **`-w <file>`** | 写入 pcap/pcapng，**避免**二进制刷屏终端 |

### 3. 限制数量（Count）

| 参数 | 说明 |
|------|------|
| **`-c <N>`** | 抓满 N 个包后自动停止 |

```bash
tshark -i 1 -c 10 -w ten.pcapng
sudo tcpdump -i eth0 -c 10 -w ten.pcap
```

### 4. 读取离线文件（Read）

| 参数 | 说明 |
|------|------|
| **`-r <file>`** | 分析已保存文件 |

```bash
tshark -r out.pcapng
tcpdump -r out.pcap
```

## 抓包/实操记录

| 场景 | 示例 |
|------|------|
| 服务器试抓 | `sudo tcpdump -i any -c 100 -w /tmp/sample.pcap` |
| Windows | `tshark -i 2 -c 50 -w C:\temp\sample.pcapng` |
| 只抓不写屏 | 务必加 `-w`；需要看摘要时去掉 `-w` 并配合 [§6.4](./04-control-output.md) |

## 疑问与总结

- **`-ni`**：TShark 常组合 `-n`（禁解析）+ `-i`；tcpdump 用 **`-nni`**（见 §6.5、6.6）。
- 与 GUI [Capture Options](../chapter-04-capture-packet/04-advanced-capture-options.md) 的 Ring Buffer 对应：tcpdump 可用 `rotatelogs` 等外部方案。
