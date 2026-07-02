# 8. 工具选型速查（HFT 优先）

| 症状 | 优先工具 |
|------|----------|
| 延迟尖刺、怀疑网络 | **`tcpretrans`** |
| 谁在用带宽 | `tcptop`、`socketio` |
| 意外 outbound 连接 | `tcpconnect`、`soconnect` |
| 连接/会话行为 | **`tcplife`** |
| 连接建立慢 | `soconnlat`、`tcpconnect` |
| 首包慢 | `so1stbyte` |
| DNS 拖慢 | **`gethostlatency`** |
| SYN 丢/满 | `tcpsynbl` |
| 接收缓冲溢出 | `sormem` |
| 内核 drop 不知因 | **`skbdrop`** |
| NIC TX 排队 | `nettxlat`、`ethtool -S` |
| 抓包替代（低开销） | `tcplife` + `tcpretrans` |

---
