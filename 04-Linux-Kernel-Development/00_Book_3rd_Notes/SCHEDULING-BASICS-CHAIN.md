# 调度基础连贯阅读（4.1 → 4.2 → 精读）

> **用途：** 把「抢占演进」与「CFS/RT 策略」收成一条线，避免章节跳读断层。  
> **章节正文：** [4.1](./chapter-04-process-scheduling/notes/section-4.1-多任务与调度器演进.md) · [4.2](./chapter-04-process-scheduling/notes/section-4.2-调度策略.md) · [4.3](./chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md) · [4.6](./chapter-04-process-scheduling/notes/section-4.6-实时调度策略.md)

---

## 阅读顺序

```
4.1  抢占式多任务 · runqueue · O(1)→CFS 演进
        │
        ▼
4.2  谁优先 / 跑多久 · I/O vs CPU · nice vs RT · 策略常量
        │
        ├──────────────► 4.3  CFS 深挖（vruntime / 红黑树）
        │
        └──────────────► 4.6  FIFO/RR · 软实时 · HFT 三板斧
                              │
                              ▼
                         4.5 / 4.7  抢占切换 · syscall/affinity
```

---

## 一页合并记忆

| 问题 | 答案锚点 |
|------|----------|
| 系统会不会被死循环卡死？ | **抢占**（4.1）；协作式才会 |
| 就绪任务在哪？ | **runqueue** / CFS 树（4.1） |
| 网络服务为何默认还行？ | CFS + **vruntime** 偏爱常休眠者（4.2/4.3） |
| nice 能否当实时用？ | **不能**；RT 另套 1–99（4.2） |
| HFT 热路径？ | 隔离核 + **FIFO** + 中断规划；勿滥用（4.2/4.6） |
| 和 fork/exec 关系？ | fork 造 task；exec 换 ELF；调度挑已有 task（[PROCESS-IDENTITY…](./PROCESS-IDENTITY-FD-FORK-EXEC.md)） |

---

## 两大阵营（4.2 核心）

```
可运行任务？
    │
    ├─ 有 RT（FIFO/RR）可运行 ──► 跑 RT（压过全部 CFS）
    │
    └─ 仅 CFS ────────────────► 选 vruntime 最小
```

---

→ [Ch4 README](./chapter-04-process-scheduling/) · [OUTLINE HFT 精读序](./OUTLINE.md)
