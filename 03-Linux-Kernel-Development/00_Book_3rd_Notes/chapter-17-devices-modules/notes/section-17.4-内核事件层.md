## ④ 内核事件层 · Kernel Events Layer

建立在 **kobject** 之上的 **内核 → 用户** 通知。

| 模型 | 从某 **kobject**（对应 **sysfs 路径**）发出 **事件** |
|------|------------------------------------------------------|
| 动作字符串 | 如 **`add`**、**`remove`** |
| 传递 | **netlink** 套接字 → 用户态（**udev/systemd-udevd**） |

```
热插拔 U 盘
    ▼
内核 kobject 注册 + uevent("add")
    ▼
udev 监听 netlink ──► 创 /dev 节点、挂载策略…
```

| 用户态 | **udev rules** — 绑 IRQ、权限、符号链接 |

---
