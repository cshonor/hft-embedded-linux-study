# Ch 13 §3 选择要杀死的进程 (`select_bad_process` / `badness`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 3. 选择要杀死的进程 (`select_bad_process` / `badness`)

遍历进程，**`badness()` 打分** — **最高分** 为牺牲品。

### 打分（原书算法直觉）

```
badness ∝  Total VM（虚拟内存占用大）
         ─────────────────────────────
         sqrt(CPU 运行时间)           （运行久 → 分母大 → 分数低）
```

| 倾向 | 原因 |
|------|------|
| **杀大内存、短寿命进程** | 突然 **malloc 暴涨** 的 **更可疑** |
| **少杀长期 daemon** | 老进程 **不一定是** 本次 OOM 根因 |

### 权限保护（分数 ÷ 4）

| 进程 | 保护 |
|------|------|
| **root** | 降低被杀概率 |
| **`CAP_SYS_ADMIN`** | 同上 |
| **`CAP_SYS_RAWIO`** | 直接硬件访问 — **更不应误杀** |

**现代：** **`/proc/<pid>/oom_score_adj`**（-1000 ~ 1000）— 用户态 **显式调节**；**cgroup** 可 **禁用/隔离** OOM。

**HFT：** 关键进程设 **`oom_score_adj=-1000`**（或 cgroup **oom_kill_disable**）— **不能防 swap/reclaim stall**，只防 **被选中杀死**。

---
