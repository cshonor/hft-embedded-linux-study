# 3. 传统磁盘分析工具

### `iostat`

```bash
iostat -dxz 1
```

| 字段 | 关注 |
|------|------|
| `rkB/s` / `wkB/s` | 吞吐 |
| `%util` | 设备忙碌程度（SSD 上勿绝对化） |
| **`await`** | **平均请求时间**（含排队+服务） |
| `r_await` / `w_await` | 读/写分项 |

**局限：** **均值掩盖长尾** — 双峰分布（设备 cache hit vs miss）在平均值里「看起来还行」。

### `perf` 与 `blktrace`

| 工具 | 特点 |
|------|------|
| `perf trace` block 事件 | 可追踪 `block_rq_*` |
| **`blktrace`** | 极详细，**busy 系统上开销与日志量巨大** |

**BPF 定位：** 介于 `iostat`（太粗）与 `blktrace`（太重）之间。

---
