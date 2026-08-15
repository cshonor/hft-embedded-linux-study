## ⑥ 内核开发社区

| 要点 | 说明 |
|------|------|
| **参与** | 全球协作 · 读 **邮件列表** · 提 **补丁** |
| **LKML** | **Linux Kernel Mailing List** — 主论坛 · **Linus 与维护者** 在场 |
| **学习法** | **读源码 + 动手改** — 优于只啃概念 |

→ 本书收官：[Ch](../../chapter-20-patches-community/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** LKML（Linux Kernel Mailing List）在内核开发中扮演什么角色？

<details><summary>答案</summary>

LKML 是内核开发的唯一官方论坛。所有补丁必须发到 LKML 接受 review，Linus 和子系统维护者在此决策。HFT 工程师如果发现内核 bug 或需要新特性，正确路径是：读源码确认问题 → 写补丁 → 发 LKML → 回应 review → 通过 maintainer 进入 mainline。

</details>

**Q2.** 为什么「读源码 + 动手改」比只啃概念书更有效？

<details><summary>答案</summary>

内核代码是最终的 ground truth。LKD 书基于 2.6 内核，现代 5.x/6.x 已大改（如 CFS→EEVDF 调度器、io_uring、BPF）。只读书会停留在过时认知；读源码能发现实际数据结构和算法，改代码能验证理解是否正确。

</details>

</details>
---
