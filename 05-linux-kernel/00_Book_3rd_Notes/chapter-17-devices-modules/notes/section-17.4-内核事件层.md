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



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核事件层（uevent）如何通知用户态？HFT 热插拔网卡如何感知？

<details><summary>答案</summary>

内核通过 kobject_uevent 发送 uevent（ADD/REMOVE/CHANGE），经 netlink(KOBJECT_UEVENT) 广播到用户态。用户态 udev 守护进程接收 uevent → 执行规则（重命名网卡/加载驱动/配置 IP）。HFT 系统可以监听 netlink uevent 感知网卡热插拔，自动重新初始化行情通道。

</details>

</details>
---
