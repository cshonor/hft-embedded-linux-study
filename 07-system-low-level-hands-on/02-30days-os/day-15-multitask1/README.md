# Day 15 · 多任务（1）


> **原书第十五章** · **并发里程碑** — **TSS/TR**、**far-JMP** 切换、定时器 **0.02s 轮转**、栈上传参、**ISR 内 `mt_taskswitch`**。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① TSS** | 单核 **伪并行** · 寄存器保存 | **TSS + TR** · **far-JMP** |
| **② 定时器切换** | **0.02s** 超时互跳 | 任务 B **窗口计数** 验证 |
| **③ 传参优化** | **`sht_back` 给 task B** | **TSS 初始栈 `[ESP+4]`** · 少 refresh |
| **④ 真多任务** | **`mt_taskswitch` 进 timer ISR** | 应用 **无感调度** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 多任务原理 | [notes/section-15.1-多任务原理.md](./notes/section-15.1-多任务原理.md) |
| 定时器驱动 | [notes/section-15.2-定时器驱动.md](./notes/section-15.2-定时器驱动.md) |
| 传参技巧 | [notes/section-15.3-传参技巧.md](./notes/section-15.3-传参技巧.md) |
| 真正的多任务 | [notes/section-15.4-真正的多任务.md](./notes/section-15.4-真正的多任务.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 单核如何「同时」？ | **快速切换** ~10–30 ms 级时间片 |
| 寄存器谁存？ | **TSS**；**TR** 指向当前；**far-JMP** 触发硬件 save/restore |
| 谁触发轮转？ | 先 **0.02s 定时器** 互跳 |
| 参数怎么给 B？ | 先 **0x0fec** → 后 **TSS 初始栈 [ESP+4]** |
| 何谓「真」多任务？ | **`mt_taskswitch` 在 timer ISR** — 应用无感 |
| 验证？ | task B **独立窗口计数** 持续增加 |

---

---

## 本日学习目标 · 自检

- [ ] 说清单核 **并发 vs 并行**
- [ ] 解释 **TSS / TR / far-JMP** 分工
- [ ] 描述 **0.02s A↔B** 与 **task B 计数窗口**
- [ ] 理解 **初始栈传参** 与固定地址中转的差别
- [ ] 对比 **应用自切换 vs ISR 内 `mt_taskswitch`**

---

← [Day 14](./day-14-高分辨率及键盘输入.md) · [01 导读](../README.md) · [Day 16](./day-16-多任务2.md)

---

## 相关

- 上一日：[../day-14-keyboard/](../day-14-keyboard/)
- 下一日：[../day-16-multitask2/](../day-16-multitask2/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
