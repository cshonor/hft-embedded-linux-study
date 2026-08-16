# Ch13 · Storage（存储）

> **Level 2 · 相知** · 策略：**🔴 精读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 小节清单（骨架，待充实）

- [ ] malloc 家族与 realloc 陷阱
- [ ] **存储期/生命周期/可见性**（static/thread/allocated/auto 四种存储期）
- [ ] 初始化规则全表
- [ ] 机器模型 digression（寄存器/内存抽象）

## HFT / DPDK 关联

HFT 热路径不用 malloc：启动时一次性分配 + 自管理内存池；四种存储期是理解 _Thread_local 数据的基础

## 自测题（待补）

<details><summary>1. （待补充）</summary>

（待补充）
</details>

---

> 本文件为章节骨架。读书时按仓库体例充实：概念 + 代码 + HFT 关联 + 自测题（`<details>` 折叠答案）。
