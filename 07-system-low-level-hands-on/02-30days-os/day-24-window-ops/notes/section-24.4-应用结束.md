## ④ 应用结束 · 自动清理定时器

#### Bug

**noodle** 设了 timer → 用户 **提前 × 关 app** → timer 仍 fire → 超时数据进 **Console FIFO** → **乱码**。

#### 修复

**定时器结构加「属主 task」标记** — app **正常退出 / 强杀 / × 关闭** 时：

```
遍历该 task 的所有 timer → cancel/free
```

**资源生命周期 = task 生命周期** — 与 Day 23 **sheet 清理** 同一原则。

**HFT：** **进程 exit 关 fd、join 线程、取消 pending alarm** — 防 **use-after-free / 幽灵回调**。

---
