## ② Oops

**Oops** = 内核报告 **无法处理的异常**（如 **空指针解引用**）。

| 输出内容 | 错误信息 · **寄存器** · **调用栈 backtrace** |
|----------|-----------------------------------------------|

#### 致命程度

| 发生位置 | 后果 |
|----------|------|
| **中断上下文**、**idle (pid 0)**、**init (pid 1)** | 无法继续 → **`panic()`** · **整机挂死** |
| **普通用户进程** 上下文 | 通常 **杀死该进程** · 内核 **尝试继续** |

#### 解码 Oops

| 时代 | 工具 |
|------|------|
| 早期 | **`ksymoops`** + **`System.map`** — 手动 **地址 → 符号** |
| **2.6+ `kallsyms`** | `CONFIG_KALLSYMS` — 符号表编进内核 → **直接可读 backtrace** |

```
Oops: 0000 [#1] SMP
Call Trace:
 [<ffffffffa0123456>] my_drv_ioctl+0x42/0x100 [mydrv]
 ...
```



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Oops 信息中最重要的字段是什么？如何从 Oops 定位源码？

<details><summary>答案</summary>

关键字段：1) RIP（出错指令地址）→ addr2line 或 objdump 定位源码行；2) Call Trace（调用栈）→ 定位调用链；3) Code:（出错指令前后的十六进制）→ 反汇编。`addr2line -e vmlinux <RIP地址>` 定位源码行。`gdb vmlinux` + `list *(RIP地址)` 查看源码。HFT 驱动 Oops 分析是最基本的内核排障技能。

</details>

**Q2.** Oops 和 panic 的区别？什么时候 Oops 不会变成 panic？

<details><summary>答案</summary>

Oops = 杀死出错进程/线程，系统可能继续运行（如果损坏不严重）。panic = 系统停止，不可恢复。Oops → panic 的条件：1) 在中断上下文中出错（无进程可杀）；2) 损坏关键内核数据结构；3) panic_on_oops 设置为 1。生产 HFT 系统通常设 panic_on_oops=1（损坏的内核不安全，宁可重启）。

</details>

</details>
---
