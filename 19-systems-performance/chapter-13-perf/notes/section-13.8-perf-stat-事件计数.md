## 13.8 `perf stat` — 事件计数

### 用途与特点

| 特点 | 说明 |
|------|------|
| **Counting** | 非采样 — 统计事件 **总次数** |
| **低开销** | 适合生产长时间跑 |
| **范围** | 全局 `-a` 或进程 `-p PID` |

### 高级选项

| 选项 | 作用 |
|------|------|
| `-e EVENT1,EVENT2` | 指定事件 |
| `-I 1000` | **每 1000ms 间隔** 打印一行 — 看趋势 |
| `-A` / `--no-aggr` | **每 CPU** 分开 — 负载均衡 |
| `--filter` | 内核/用户过滤 `u`/`k` |
| `-d` | 详细 stat（更多默认事件） |
| `-r 5` | **重复 N 次** — 看方差 |

```bash
# 每 CPU 每秒 IPC 趋势
perf stat -e cycles,instructions -I 1000 -a -- sleep 5

# 仅用户态
perf stat -e cycles,instructions -u -p $(pidof strategy) -- sleep 10
```

**Shadow Statistics：** 某些环境下 perf 用 **影子计数** 减干扰 — 详见 `man perf-stat`；理解「数字从哪来」即可。

**HFT 验收：** 绑核/调优前后 **`perf stat` 固定事件集** — 存档对比。

---


### 常见陷阱

1. perf stat 只跑一次——单次有抖动，应用 -r 5 重复 5 次看方差
2. 不区分用户态/内核态——-u 只看用户态、-k 只看内核态，混在一起看不出是策略慢还是内核慢
3. 不看 per-CPU——-A 每核分开，HFT 热核和其他核可能差很大

<details>
<summary>自测题（点击展开）</summary>

1. perf stat 的 -r 5 选项有什么用？
   <details><summary>答</summary>重复 5 次看方差——单次有抖动，重复后看 std dev 判断稳定性</details>
2. 如何区分用户态和内核态开销？
   <details><summary>答</summary>-u 只看用户态（策略/解码）、-k 只看内核态（syscall/协议栈）——分别排查</details>
3. HFT 为什么要用 -A（per-CPU）模式？
   <details><summary>答</summary>热核和其他核可能差很大——全局平均掩盖热核问题</details>

</details>


---

← [本章导读](../README.md)
