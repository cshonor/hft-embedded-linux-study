# Ch21 · Atomic access and memory consistency（原子访问与内存一致性）

> **Level 3 · 深入** · 策略：**🔴 精读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 小节清单（骨架，待充实）

- [ ] data race 与 UB
- [ ] **happens-before 关系**
- [ ] **内存序：seq_cst / acquire / release / acq_rel / relaxed**
- [ ] 原子操作 vs 锁的对比
- [ ] 内存屏障与一致性模型

## HFT / DPDK 关联

全书压轴、DPDK rte_ring 的理论基础。rte_ring 的 producer/consumer 就是 acquire/release 序；relaxed 用于计数器。读懂本章才能读懂无锁队列源码

## 自测题（待补）

<details><summary>1. （待补充）</summary>

（待补充）
</details>

---

> 本文件为章节骨架。读书时按仓库体例充实：概念 + 代码 + HFT 关联 + 自测题（`<details>` 折叠答案）。
