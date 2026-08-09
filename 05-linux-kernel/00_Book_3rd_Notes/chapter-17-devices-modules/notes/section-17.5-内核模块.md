## ⑤ 内核模块 · Modules

Linux = **宏内核**，但支持 **可加载模块** — 运行时 **插入/移除** 对象代码。

| 作用 | 说明 |
|------|------|
| **设备驱动** | 按需 `modprobe` — 无硬件时不占内核 |
| **热插拔** | 总线探测 → 加载对应模块 |
| 不限于驱动 | 文件系统、协议等也可模块化 |

| 用户命令 | 说明 |
|----------|------|
| **`insmod` / `modprobe`** | 加载 |
| **`rmmod`** | 卸载 |
| **`lsmod`** | 列出 |

→ **Ch 2** 编译安装 · `make modules_install` → `/lib/modules/`

```bash
# 概念
modprobe ixgbe          # 加载网卡驱动模块
cat /sys/module/ixgbe/parameters/...
```

**HFT：** 定制 **网卡驱动模块**、**内核参数** 与 **`/sys/module/.../parameters`** — 生产变更需可回滚。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** insmod 和 modprobe 的区别？内核模块如何符号导出？

<details><summary>答案</summary>

insmod：直接加载单个 .ko 文件，不处理依赖。modprobe：自动解析依赖（读 modules.dep），按顺序加载依赖模块。模块用 `EXPORT_SYMBOL(symbol)` 导出符号到内核符号表，其他模块可调用。`EXPORT_SYMBOL_GPL` 仅 GPL 模块可用。HFT 定制驱动编译为 .ko，modprobe 加载，可运行时更新驱动不需重启。

</details>

**Q2.** 内核模块和用户态程序的区别？为什么内核模块 bug 更危险？

<details><summary>答案</summary>

内核模块运行在内核态（ring 0），有全部权限：可访问任何内存、任何硬件、任何系统调用。用户态程序运行在 ring 3，受限访问。内核模块 bug → oops/panic/安全漏洞 → 整个系统崩溃。用户态 bug → 仅该进程 crash。HFT 定制网卡驱动必须充分测试，一个空指针解引用就能让交易系统全挂。

</details>

</details>
---
