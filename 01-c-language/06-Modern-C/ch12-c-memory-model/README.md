# Ch12 · The C memory model（C 内存模型）

> **Level 2 · 相知** · 策略：**🔴 精读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 小节清单（骨架，待充实）

- [ ] 统一内存模型（所有对象都是字节数组）
- [ ] union 与类型双关
- [ ] 内存与状态（volatile 语义）
- [ ] void* 与未指定对象
- [ ] 隐式/显式转换
- [ ] **Effective Type 规则**
- [ ] **对齐 alignment（alignof/alignas）**

## HFT / DPDK 关联

本书核心章。effective type 决定能否把收到的网络字节直接 cast 成结构体（DPDK mbuf 解析的理论依据）；_Alignas(64) 防伪共享

## 自测题（待补）

<details><summary>1. （待补充）</summary>

（待补充）
</details>

---

> 本文件为章节骨架。读书时按仓库体例充实：概念 + 代码 + HFT 关联 + 自测题（`<details>` 折叠答案）。
