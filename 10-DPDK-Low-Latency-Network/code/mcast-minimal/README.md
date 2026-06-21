# 组播行情最小工程（待实现）

> DPDK 最小可运行示例：绑定网卡 → 收 UDP 组播 → 解析行情头。

## 目标

- 对照 [05 UNP](../08-UNP-Vol1/) socket 组播版本，理解旁路差异
- 对照 [06 note-组播IGMP](../09-Linux-Kernel-Networking/note-组播IGMP.md) 内核路径

## 计划结构

```
mcast-minimal/
├── README.md
├── Makefile / meson.build
└── src/
    └── main.c    # EAL init → rx_burst → parse
```

## 前置

- 大页配置、`dpdk-devbind`
- 测试网卡或 vhost-user 环境

<!-- 代码待补充 -->
