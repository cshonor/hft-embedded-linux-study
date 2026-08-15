# 6.7 TShark 里的时间显示格式

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · GUI：[§4.3](../chapter-04-capture-packet/03-time-display-format.md)

**核心主旨**：默认相对时间不利于对日志；用 `-t` 强制绝对时间对齐外部系统。

## 核心知识点

| 工具 | 默认时间 |
|------|----------|
| **TShark** | 常为**相对抓包开始**（如 `0.000000`） |
| **tcpdump** | 多为**绝对时间** |

跨设备、对 syslog/防火墙日志时，TShark 相对时间易**混淆**。

### `-t` 格式修正

```bash
tshark -r file.pcapng -t ad
```

| 标志 | 含义（常用） |
|------|----------------|
| **`-t ad`** | **Absolute Date**：`YYYY-MM-DD hh:mm:ss.ffffff` |
| 其他 | `a` 绝对秒、`r` 相对、`d` 增量等（`tshark -h` 查全表） |

示例输出：`2015-12-21 12:52:43.116551`

### 实战

| 场景 | 做法 |
|------|------|
| 与 NTP 同步日志比对 | `-t ad` + 导出字段 `frame.time` |
| 脚本 | `-T fields -e frame.time -e ip.src -e ip.dst` |

## 抓包/实操记录

```bash
tshark -r incident.pcapng -t ad -Y "tcp.flags.syn==1" \
  -T fields -e frame.time -e ip.src -e tcp.dstport
```

与服务器 `/var/log/nginx/access.log` 时间戳对齐排查。

## 疑问与总结

- 合并多 pcap 前先在 GUI 做 **Time Shift**（§4.3），CLI 用 `-t ad` 检查是否仍错位。
- 写报告时注明时间是否为 UTC 或本地时区。
