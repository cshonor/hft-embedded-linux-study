# Ch 17 设备与模块 · Devices and Modules

> **Linux Kernel Development 3rd** · Robert Love · **背景**

> 本章定位：**块/字符/网络设备** 分类、**统一设备模型**（kobject/kset/kref）、**sysfs**、**uevent**、**内核模块**。读驱动、`/sys` 调参、热插拔与 **Ch 5「用 sysfs 替代新 syscall」** 的落地页。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 设备类型** | block/char/net/misc | socket 例外 |
| **② 统一设备模型** | kobject 族 | 拓扑 · 电源管理 |
| **③ sysfs** | `/sys` | kobject → 目录/文件 |
| **④ 内核事件层** | uevent | netlink 通知用户态 |
| **⑤ 内核模块** | loadable modules | 驱动热插拔基础 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 设备类型 | [notes/section-17.1-设备类型.md](./notes/section-17.1-设备类型.md) |
| 统一设备模型 | [notes/section-17.2-统一设备模型.md](./notes/section-17.2-统一设备模型.md) |
| sysfs 虚拟文件系统 | [notes/section-17.3-sysfs-虚拟文件系统.md](./notes/section-17.3-sysfs-虚拟文件系统.md) |
| 内核事件层 | [notes/section-17.4-内核事件层.md](./notes/section-17.4-内核事件层.md) |
| 内核模块 | [notes/section-17.5-内核模块.md](./notes/section-17.5-内核模块.md) |
| 设备栈总览（与前面章节） | [notes/section-17.6-设备栈总览与前面章节.md](./notes/section-17.6-设备栈总览与前面章节.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 三种主设备？ | **块 · 字符 · 网络（socket）** |
| 网络为何特殊？ | **非**「一切皆文件」设备节点 |
| 设备模型核心？ | **kobject / ktype / kset / kref** |
| sysfs？ | kobject 树 → **`/sys`** |
| uevent？ | **netlink** 通知 udev 等 |
| 模块？ | 宏内核的 **动态扩展** · 驱动热插拔 |

---

## 本章学习目标 · 自检

- [ ] 区分 **块设备 seek** 与 **字符字节流**
- [ ] 解释 **kobject 嵌入 cdev** 的意义
- [ ] 说出 **sysfs 目录/文件** 与 kobject/属性的对应
- [ ] 描述 **uevent → udev** 热插拔链
- [ ] 知 **misc / 伪设备** 各一例
- [ ] HFT：能举 **`/sys` 下调网卡/块设备** 的路径

---

## 相关章节

- 上一章：[../chapter-16-page-cache/](../chapter-16-page-cache/)
- 下一章：[../chapter-18-debugging/](../chapter-18-debugging/)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
