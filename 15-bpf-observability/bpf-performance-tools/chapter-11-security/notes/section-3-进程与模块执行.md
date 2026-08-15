# 3. 进程与模块执行

### `execsnoop`

全系统 **`execve`** — 新进程执行。

```bash
sudo execsnoop-bpfcc
```

| 安全 | 性能（Ch 6） |
|------|--------------|
| 发现恶意/未知二进制 | 发现短命脚本消耗 CPU |

### `elfsnoop`

追踪 **ELF 二进制加载** — 比 `execsnoop` 更贴近内核 ELF 处理路径，抓 **隐蔽加载**。

```bash
sudo elfsnoop-bpfcc
```

### `modsnoop`

监控 **内核模块加载**（`modprobe` 等）— **Rootkit** 常见手法。

```bash
sudo modsnoop-bpfcc
```

**HFT 共置机：** 交易节点不应动态加载未知 **ko** — 非零输出即告警。

---
