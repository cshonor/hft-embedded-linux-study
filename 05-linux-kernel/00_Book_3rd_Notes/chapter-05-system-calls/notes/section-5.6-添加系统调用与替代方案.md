## ⑥ 添加系统调用与替代方案

---

### 方式 1：传统新增自定义系统调用（现代内核不推荐）

概念步骤：

| 步骤 | 动作 |
|------|------|
| 1 | 在 `uapi/.../unistd.h` 等处分配新系统调用号 |
| 2 | 用 `SYSCALL_DEFINE` 实现内核函数 |
| 3 | 在 `sys_call_table` 注册 |
| 4 | **重新编译** Linux 内核 |

| 缺点 | 说明 |
|------|------|
| 改动内核源码 | 升级内核易失效 / 难维护 |
| 系统调用号稀缺 | 全局 ABI 资源 |
| 无法动态加载 | 必须整体编进内核 |
| 主线一旦发布 | 接口 **永久兼容** — 「刻在石头上」 |

无 libc 封装时，历史上可用 **`_syscalln()`** 宏系列从用户态直接发起（教学用，生产少见）。

---

### ✅ 现代推荐替代方案（工作中首选）

| 方案 | 适用 |
|------|------|
| **ioctl** | 打开自定义设备 `/dev/xxx`，用命令码传请求；适合驱动交互 |
| **Netlink Socket** | 双向异步消息；模块可动态注册，**不用改** 系统调用表 |
| **sysfs / procfs** | 文件形式读写字符串/参数；适合简单配置 |
| **eBPF** | 较新路径：可编程观测/部分数据面，仍走既有内核接口体系 |
| 字符设备 + `read`/`write` | 流式或简单命令 |

> **工程准则：尽量不要新增自定义系统调用。**

**HFT 工程：** 生产 rarely 改内核 syscall；调优多用 **已有接口**（`epoll`、`mmap`、`setsockopt`、netlink）或 **内核模块 / 驱动**。

→ 收官：[Ch 20 补丁与社区](../../chapter-20-patches-community/) · [P3.5 BusyBox 实操](../../../../projects/P3.5-busybox-minimal-linux/) · 回 [§5.1](./section-5.1-与内核通信.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 如果要在内核中添加一个新系统调用，需要修改哪些文件？

<details><summary>答案</summary>

1) `arch/x86/entry/syscalls/syscall_64.tbl` 添加 syscall 号；2) `include/linux/syscalls.h` 添加原型声明；3) `kernel/xxx.c` 实现函数（SYSCALL_DEFINE2(my_call, int, arg, ...)）；4) 重新编译内核；5) 用户态通过 syscall() 或 libc 包装调用。现代内核不推荐这种方式。

</details>

**Q2.** eBPF 为什么比新增系统调用更灵活？

<details><summary>答案</summary>

新增 syscall 是静态的：编译时固定、升级内核才能改。eBPF 是动态的：用户态程序运行时加载字节码到内核，内核验证安全性后执行。不需要改内核源码、不需要重启。HFT 可以用 eBPF 动态追踪内核行为（如 syscall 延迟、网卡丢包）而不修改内核。

</details>

</details>
---
