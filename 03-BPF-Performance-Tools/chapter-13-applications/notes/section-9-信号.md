# 9. 信号 (Signals)

### `signals` / `killsnoop`

| 工具 | 作用 |
|------|------|
| `signals` | 追踪 **信号发送**（谁向谁发了什么） |
| `killsnoop` | 追踪 **kill** — 谁发了 **SIGKILL/SIGTERM** |

```bash
sudo killsnoop-bpfcc
```

**场景：** 策略进程 **意外退出** — OOM？人为 kill？watchdog？

---
