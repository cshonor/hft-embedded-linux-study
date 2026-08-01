# 5. 权限与能力 (Capabilities)

Linux **capabilities** 拆分 root 特权 — 最小权限设计的基础。

### `capable`

追踪 **capability 检查**（如 `CAP_SYS_ADMIN`、`CAP_NET_RAW`）。

```bash
sudo capable-bpfcc
```

| 用途 | 说明 |
|------|------|
| **安全** | 看谁在校验/使用特权 |
| **白名单** | 跑一遍合法 workload → 记录 **实际需要** 的 cap → 其余 **drop** |

**HFT：** 新服务上线前 **`capable` 短跑** → 写 systemd `CapabilityBoundingSet` / Docker `--cap-drop`。

### `setuids`

追踪 **`setuid` / `setresuid` / `setfsuid`** — 权限提升（`sudo`、`sshd` 等）。

```bash
sudo setuids-bpfcc
```

### `eperm`

统计 syscall 返回 **`EPERM` / `EACCES`** — 识别 **反复越权尝试** 的进程。

```bash
sudo eperm-bpfcc
```

**场景：** 漏洞利用探测、错误 seccomp 规则调试。

---
