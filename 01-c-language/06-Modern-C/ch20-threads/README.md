# Ch20 · Threads（线程）

> **Level 3 · 深入** · 策略：**🔴 精读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 小节清单（骨架，待充实）

- [ ] threads.h（thrd_create/join/detach）
- [ ] **线程局部数据 tss/_Thread_local**
- [ ] 临界区与互斥（mtx）
- [ ] 条件变量（cnd）
- [ ] 线程管理策略

## HFT / DPDK 关联

进 13 DPDK 前必读。DPDK 每 lcore 一线程模型 = _Thread_local/绑核；虽然生产用 pthread，threads.h 是理解模型的最短路径

## 自测题（待补）

<details><summary>1. （待补充）</summary>

（待补充）
</details>

---

> 本文件为章节骨架。读书时按仓库体例充实：概念 + 代码 + HFT 关联 + 自测题（`<details>` 折叠答案）。
