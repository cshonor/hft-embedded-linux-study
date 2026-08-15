# 6. 网络与文件异常

### `tcpconnect` / `tcpaccept`

| 工具 | 安全视角 |
|------|----------|
| `tcpconnect` | **意外 outbound** — 编译器/工具链突然外连 |
| `tcpaccept` | 不应监听的服务开始 **accept** |

→ 性能视角：[Ch 10](../../chapter-10-networking/)

```bash
sudo tcpconnect-bpfcc
sudo tcpaccept-bpfcc
```

### `tcpreset`

追踪内核发送 **TCP RST** — **端口扫描** 常见特征。

```bash
sudo tcpreset-bpfcc
```

### `opensnoop`

监控 **`open()` / `openat()`** — 是否读 **`/etc/passwd`**、密钥路径等。

```bash
sudo opensnoop-bpfcc
```

→ 性能视角：[Ch 8](../../chapter-08-file-systems/)

---
