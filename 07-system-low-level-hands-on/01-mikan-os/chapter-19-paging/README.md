# Ch 19 · 分页

> **原书第 19 章** · HFT **🔴** · 官方源码标签 `osbook_day19`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **应用虚拟化：** **链接基址错位** · **四级分页** · **`SetupPageMaps`/`CleanPageMaps`** · **rpn → 5**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 根因** | **`--image-base 0`** vs 实际加载 | **rpn 输出 0 非 5** |
| **② 方案** | **虚拟地址** · **MMU** | 不重定位 · **隔离** |
| **③ 机制** | **PML4→PDP→PD→PT** · **CR3** | Ch8 **深化** |
| **④ 实战** | **0xffff8000…** · **动态页表** · **清理** | **`rpn 2 3 +` 正确** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 链接基址与 rpn 失败 | [notes/section-2-链接基址与rpn失败.md](./notes/section-2-链接基址与rpn失败.md) |
| 3. 虚拟地址与地址转换 | [notes/section-3-虚拟地址与地址转换.md](./notes/section-3-虚拟地址与地址转换.md) |
| 4. x86-64 四级分页 | [notes/section-4-x86-64四级分页.md](./notes/section-4-x86-64四级分页.md) |
| 5. 高半区链接与 SetupPageMaps | [notes/section-5-高半区链接与SetupPageMaps.md](./notes/section-5-高半区链接与SetupPageMaps.md) |
| 6. CleanPageMaps 与小结 | [notes/section-6-CleanPageMaps与小结.md](./notes/section-6-CleanPageMaps与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **每应用专属页表** — 高半 **VA** 映射 **LOAD 段** · 跑完 **释放** |
| 与 02 川合 OS 对照？ | 01 **Day 12+ 分页**；Mikan **Ch8 身份映射 + Ch19 应用 VA** |
| 与 Linux / CSAPP 对照？ | **进程地址空间** — CSAPP Ch9 · **mmap/BRK 前奏** |

**本章目的：** 用 **分页** 解决 **链接地址 ≠ 加载地址** — **rpn 终于输出 5**

---

## 本章学习目标 · 自检

- [ ] 说清 **`--image-base 0`** 为何导致 **rpn 错**
- [ ] **VA→PA** 与 **CR3 切换** 流程
- [ ] **PML4/PDP/PD/PT** 各用 VA 哪几位
- [ ] **`SetupPageMaps` / `CleanPageMaps`** 何时调用

---

## 相关

- 上一章：[../chapter-18-apps/](../chapter-18-apps/)
- 下一章：[../chapter-20-syscall/](../chapter-20-syscall/)
- 前置：[../chapter-08-memory/](../chapter-08-memory/) · [../chapter-18-apps/](../chapter-18-apps/)
