## 6.4.2 直接映射 (E=1)

> **Ch6 §6.4.2** · [章导读](../README.md) · 上节 [§6.4.1 ←](./section-6.4.1-通用组织结构.md) · 下节 [§6.4.3 →](./section-6.4.3-组相联.md)
> ↔ [Harris §8.3 高速缓存](../../../00-digital-logic-cpu/ch08_memory/8.3_高速缓存.md)

---

- 每组 **一条** line — 实现简单
- **冲突 miss (conflict miss)：** 多个不同块映射到 **同一组**，互相踢出

```
组 i 只能放 1 个块 — 交替访问 A、B 同组 →  thrashing
```

---

### 常见陷阱

1. **交替访问映射到同一组的两个地址** — 直接映射 E=1，每组只有 1 条 line。如果地址 A 和 B 的 set index 相同，交替访问会导致**反复踢出**（thrashing），每次都 miss。
2. **以为直接映射没人用** — 实际上 L1 常用直接映射或低路数组相联（速度优先）。直接映射查找最快（只需 1 次 tag 比较），但冲突 miss 高。
3. **混淆 conflict miss 和 capacity miss** — conflict miss 是因为相联度不够（即使总容量够，同组装不下）；capacity miss 是因为总容量不够。直接映射的 conflict miss 最多。

### 自测题

<details>
<summary>1. 直接映射（E=1）的优缺点是什么？</summary>

**优点**：实现简单、查找最快（只需 1 次 tag 比较）、功耗低。**缺点**：冲突 miss（conflict miss）高——多个不同块映射到同一组时互相踢出（thrashing），即使总容量够也 miss。
</details>

<details>
<summary>2. 什么情况会导致直接映射的 thrashing？</summary>

交替访问两个 set index 相同的地址。例如 `data[0]` 和 `data[64]`（如果数组步长恰好让它们映射到同一组），交替访问时每次都把对方踢出，**每次都 miss**，CPE 暴涨。
</details>

<details>
<summary>3. conflict miss 和 capacity miss 有什么区别？</summary>

**Conflict miss**：总容量够，但相联度不够——同组的 line 装不下多个不同块。**Capacity miss**：总容量不够——工作集超过 cache 容量。直接映射（E=1）的 conflict miss 最多；全相联（E=所有）消除了 conflict miss。
</details>

---

← [§6.4.1 ←](./section-6.4.1-通用组织结构.md) · [本章导读](../README.md) · [§6.4.3 →](./section-6.4.3-组相联.md)
