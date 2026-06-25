# 6. 调度器与饱和度

### `runqlat` — 运行队列延迟 🔴

测量：**线程进入 RUNNABLE → 实际在 CPU 上运行** 的等待时间分布。

```bash
sudo runqlat-bpfcc 10          # 每 10s 打印直方图
sudo runqlat-bpfcc -P 1 10     # 仅 CPU 1（绑核核对）
```

| 解读 | 含义 |
|------|------|
| 直方图右尾拉长 | CPU **饱和** — 就绪线程排队 |
| 绑核 dedicated 核接近 0 | 健康 |
| 突刺与行情峰值对齐 | 可能争抢同核、或邻居进程干扰 |

**Gregg 观点：** 排队 **时间 (latency)** 比排队 **长度 (length)** 更直接反映性能影响 — 但仍可用 `runqlen` 辅助。

### `runqlen`

**采样** 各 CPU 运行队列 **长度**（有多少线程在等）。

```bash
sudo runqlen-bpfcc 5
```

### `runqslower`

仅打印 **等待超过阈值** 的线程（如 >10ms）— 适合抓 **长尾**，避免海量输出。

```bash
sudo runqslower-bpfcc 10       # 10ms
```

**HFT runbook：** incident 时 **先 `runqlat` 10s** → 若右尾异常再 `runqslower` 抓具体 PID/栈。

---
