# 4. 终端与 Shell 监控（实时取证）

### `bashreadline`

静默追踪 **Bash `readline()`** — 抓取交互式输入命令（含受限 shell 场景）。

```bash
sudo bashreadline-bpfcc
```

### `shellsnoop`

**镜像** 指定 Shell 会话的 **STDOUT/STDERR** — 看攻击者屏幕输出。

### `ttysnoop`

在 **TTY/PTS 设备层** 监视会话 — 适用于 **SSH 登录** 实时旁观。

**HFT：** 仅 **堡垒机/运维跳板** 场景；**策略服务器** 不应有交互 shell — 工具用于 **事故调查**，非常态监控。

---
