## ③ 内核调试选项 · Kernel Hacking

`make menuconfig` → **Kernel Hacking**（依赖 **`CONFIG_DEBUG_KERNEL`**）

| 功能示例 | 作用 |
|----------|------|
| **sleep-inside-spinlock 检测** | 在 **原子上下文**（持 spinlock / 关抢占）**非法睡眠** → 抓 **死锁元凶** |

→ **Ch 9–10** 自旋锁 vs mutex 上下文规则

| 现代补充 | **LOCKDEP**、**KASAN**、**KFENCE** — 书中未详述，方向一致 |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** CONFIG_DEBUG_INFO 有什么用？为什么 HFT 内核应该开启？

<details><summary>答案</summary>

CONFIG_DEBUG_INFO 在编译时保留调试符号（DWARF），vmlinux 包含完整的函数/变量/行号信息。开启后：1) Oops 的 addr2line 能精确定位源码行；2) gdb 调试内核（kgdb）有符号信息；3) crash 工具分析 vmcore。代价：vmlinux 体积增大 ~10x（但运行时无影响）。HFT 生产内核应开启（debuginfo 分离存储，不影响运行时性能）。

</details>

</details>
---
