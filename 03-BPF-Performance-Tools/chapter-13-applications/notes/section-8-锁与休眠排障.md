# 8. 锁与休眠排障

### `pmlock` / `pmheld`

针对 **`libpthread` mutex**：

| 工具 | 回答 |
|------|------|
| **`pmlock`** | **锁竞争** 栈 + 等待延迟 |
| **`pmheld`** | **谁持有锁** + 持有时长 |

```bash
sudo pmlock-bpfcc -p $(pidof myapp) 10
sudo pmheld-bpfcc -p $(pidof myapp) 10
```

**HFT：** 共置服务争用 **同一把全局 mutex** 时极有用；策略热路径应 **无 pmlock 热点**。

### `naptime`

追踪 **`nanosleep(2)`** — 揪出代码中的 **显式休眠**。

```bash
sudo naptime-bpfcc -p $(pidof myapp)
```

---
