# 5. 内核锁分析

### `mlock` / `mheld`

针对 **内核 mutex**（非 pthread — 见 [Ch 13 `pmlock`](../../chapter-13-applications/)）：

| 工具 | 作用 |
|------|------|
| **`mlock`** | mutex **获取延迟** 直方图 + **内核栈** |
| **`mheld`** | **持有者栈** + **持有时长** |

```bash
sudo mlock-bpfcc 10
sudo mheld-bpfcc 10
```

### 自旋锁

| 建议 | 原因 |
|------|------|
| **勿 kretprobe 自旋锁** | 安全/稳定 |
| 用 **`profile`** | 找 **CPU 自旋** 热点 |

**HFT：** 内核侧 spin 在 **网络驱动、极端锁** 场景；用户态热路径应不可见 — 若 profile 内核栈 spin 多 → 驱动/内核版本问题。

---
