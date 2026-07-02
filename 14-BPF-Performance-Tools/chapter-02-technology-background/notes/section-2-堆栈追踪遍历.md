# 2. 堆栈追踪遍历 (Stack Trace Walking)

理解事件 **从哪条代码路径来** — `profile`、`offcputime`、栈采样都依赖栈回溯。

| 方法 | 原理 | 备注 |
|------|------|------|
| **帧指针 (Frame Pointer)** | x86-64：`RBP` 链 + 固定偏移 walk 栈帧 | **最快**；需 `-fno-omit-frame-pointer`（或 distro 默认保留） |
| **DWARF / debuginfo** | 调试信息解析栈 | 准但慢、需安装 debug 包 |
| **LBR** (Last Branch Record) | CPU 硬件记录最近分支 | 深度有限；Intel 常用 |
| **ORC** (Oops Rewind Capability) | 内核 unwind 元数据 | 内核栈常用；与用户态 DWARF 互补 |

> **HFT：** 发布二进制若 **省略帧指针**，火焰图会出现 `<unknown>` 或错误栈 — 与 [SysPerf Ch 6 CPU](../../../13-Systems-Performance-2nd/chapter-06-cpus/) 的 `-g` / FPO 讨论同构。策略 SO 建议 **保留 frame pointer** 或配 USDT/静态探针。

```bash
# 检查内核是否启用 ORC（现代发行版常见）
grep CONFIG_UNWINDER_ORC /boot/config-$(uname -r)
```

---
