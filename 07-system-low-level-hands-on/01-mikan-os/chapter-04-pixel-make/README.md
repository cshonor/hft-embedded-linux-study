# Ch 4 · 像素绘图和 make 入门

> **原书第 4 章** · HFT **⚪** · 官方源码标签 `osbook_day04`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **工程化 + 绘图：** Makefile 自动化 · **PixelWriter** · **ELF 加载器修正**

---

### 本章双线结构

| 线 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 构建** | **make / Makefile** | 告别手写 `clang++` / `ld.lld` |
| **② 绘图** | `WritePixel` → **PixelWriter** | RGB/BGR · 消除热路径分支 |
| **③ 加载** | ELF 程序头 · **LOAD 段** | 修复 Ch3 内存大小计算 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. make 与 Makefile | [notes/section-2-make与Makefile.md](./notes/section-2-make与Makefile.md) |
| 3. 像素格式与 WritePixel | [notes/section-3-像素格式与WritePixel.md](./notes/section-3-像素格式与WritePixel.md) |
| 4. PixelWriter、vtable 与 Placement new | [notes/section-4-PixelWriter与vtable.md](./notes/section-4-PixelWriter与vtable.md) |
| 5. ELF 格式与加载器改进 | [notes/section-5-ELF格式与加载器改进.md](./notes/section-5-ELF格式与加载器改进.md) |
| 6. 小结与索引 | [notes/section-6-小结与索引.md](./notes/section-6-小结与索引.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **Makefile** 构建 · **PixelWriter** 任意坐标上色 · **正确 ELF LOAD 加载** |
| 与 02 川合 OS 对照？ | 01 **Day 3+ Makefile** 较早出现；Mikan 在 **内核 C++ + GOP** 语境下引入 |
| 与 Linux / CSAPP 对照？ | ELF 加载 ≈ **exec 前映像布局** 极简版；vtable ≈ **多态实现成本** |

**本章目的：** 提升 **工程化与绘图性能**，**理顺内核加载底层逻辑**。

---

## 本章学习目标 · 自检

- [ ] 能写基本 **Makefile**（目标、依赖、命令）
- [ ] 说清 **RGB vs BGR** 及 GOP 像素格式
- [ ] 解释 **PixelWriter + 虚函数** 如何去掉循环内分支
- [ ] 理解 **vtable** 与 **Placement new**（无堆时实例化对象）
- [ ] 描述 ELF **程序头 / LOAD 段** 及加载器 **读→算范围→拷贝→跳入口**

---

## 相关

- 上一章：[../chapter-03-bootloader-display/](../chapter-03-bootloader-display/)
- 下一章：[../chapter-05-console-text/](../chapter-05-console-text/)
- 附录：[appendix-D C++ 模板](../appendix-D-cpp-templates/)
- 模块导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
