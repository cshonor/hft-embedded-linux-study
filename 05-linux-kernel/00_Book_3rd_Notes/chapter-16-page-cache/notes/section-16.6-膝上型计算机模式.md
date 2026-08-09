## ⑥ 膝上型计算机模式 · Laptop Mode

| 目标 | **硬盘尽量停转** — 省电 |
|------|-------------------------|
| 行为 | 除超时脏页外，在磁盘 **因其他 I/O 已转** 时 **搭便车** 写回 **全部脏缓冲** — 避免 **专为写回再启动** 硬盘 |

| 场景 | 笔记本 · 非 HFT 实盘常态 — 了解即可 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Laptop Mode 对 HFT 有什么启发？

<details><summary>答案</summary>

Laptop Mode 通过延迟写回让磁盘长时间停转省电。HFT 启发：1) 如果交易日志用 NVMe（无机械部件），Laptop Mode 无意义；2) 但原理可借鉴——批量写回减少 IO 次数。HFT 可以调高 vm.dirty_writeback_centisecs 让 flusher 少运行，减少对交易线程的干扰。

</details>

</details>
---
