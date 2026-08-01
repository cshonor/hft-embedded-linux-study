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

---
