## ② Linux 的诞生 · Along Came Linus

| 项 | 内容 |
|----|------|
| **时间** | **1991** · 赫尔辛基大学 · **Linus Torvalds** |
| **动机** | 当时 **缺乏强大且免费的 Unix** |
| **性质** | **非商业** · **互联网协作** 开发 |
| **许可证** | **GNU GPL 2.0** — **自由 / 开源软件** |

**与 HFT 实盘：** 生产内核多为 **发行版稳定内核 + 厂商补丁** — 理解 **GPL 与源码可得** 有助于合规使用 **定制内核/驱动**。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 采用 GPL v2 许可证，这对商用 HFT 公司有什么影响？

<details><summary>答案</summary>

GPL v2 要求修改后的内核代码必须开源。HFT 公司的定制内核调度器/网卡驱动如果分发给客户就需要开源；但只在内部使用不分发则不触发 GPL 义务。许多 HFT 公司选择只在内部维护定制内核。

</details>

**Q2.** Linus 为什么选择宏内核而非微内核？

<details><summary>答案</summary>

宏内核性能好：syscall 不需要跨进程消息传递，直接函数调用。微内核的理论优势（模块化、隔离）在实践中被 Linux 的动态模块加载（insmod/rmmod）和命名空间部分弥补。Linus 务实优先于理论纯粹性。

</details>

</details>
---
