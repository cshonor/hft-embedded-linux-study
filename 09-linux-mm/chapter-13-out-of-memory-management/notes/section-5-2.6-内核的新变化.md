# Ch 13 §5 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 5. 2.6 内核的新变化

| 变化 | 目的 |
|------|------|
| **`VM_ACCOUNT` VMA**（Ch 12） | 对 **shmem 等** 操作 **额外检查** 可用内存 |
| **严格 overcommit 上限** | **`committed_AS ≤ 总RAM × (overcommit_ratio/100) + 总swap`** — **尽量不让系统走到 OOM killer** |

**哲学：** **记账前移** — 在 **承诺内存时** 就拒绝，而非 **用尽后杀人**。

---

## OOM 决策简图

```
alloc / brk / mremap 大请求
    vm_enough_memory? ──否──► 可能 early fail（视 overcommit）
        │
内存压力 ↑
    kswapd + direct reclaim 尽力
        │
仍失败 → out_of_memory()
    防误杀检查 ──跳过──► 返回，稍后重试
        │
    select_bad_process() / badness
        │
    SIGKILL / SIGTERM → 进程退出 → 页框归还
```

---

## HFT 精读 checklist

| 手段 | 目的 |
|------|------|
| **足够物理 RAM** | 根本 — OOM = 已 **设计失败** |
| **`mlock` 关键映射** | 减 **RSS 被回收**；**不防** 他进程 OOM |
| **`oom_score_adj=-1000`** | 降低 **交易主进程** 被杀概率 |
| **cgroup memory limit** | 隔离 **非关键** 服务，**别拖全局 OOM** |
| **监控 dmesg / kernel oom** | 事后 **comm + score** |
| **Ch 1 读源码** | **`oom_kill.c`** 是 **进 mm/ 的第一站** — 串联 Ch 2 水位、Ch 10 回收、Ch 6 GFP |

---
