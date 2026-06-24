# Ch 5 · 系统调用 · System Calls

> **Linux Kernel Development 3rd** · Robert Love · **选读**  
> 本章定位：**用户态 ↔ 内核** 的合法正门 — syscall 号、`sys_call_table`、陷入路径、**参数验证**、进程上下文。HFT **少 syscall、懂延迟从哪来** 的底层一页。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 与内核通信** | API vs syscall | **机制，非策略** |
| **② 基础** | 号 · `sys_*` | **`sys_call_table`** |
| **③ 处理程序** | 陷入 · 寄存器传参 | x86 **`eax`** 号 |
| **④ 实现与安全** | 验证 · capabilities | **`copy_*_user`** |
| **⑤ 上下文** | 进程上下文 | 可睡眠 · 可抢占 · 可重入 |
| **⑥ 添加 syscall** | 绑定与替代 | **慎增新号** |

---

### ① 与内核通信 · Communicating with the Kernel

**系统调用** = 用户空间访问内核服务的 **主要合法入口**（另：**异常 / 陷入** 也可进内核，但语义不同）。

```
应用程序
    │  POSIX API（read/write/open…）
    ▼
  libc（glibc 等）── 包装 ──► 真正 syscall
    │
    ▼
  内核 sys_* 实现
```

| 层次 | 谁 |
|------|-----|
| **用户写的** | `printf` → 往往经 libc，底层或 `write` |
| **实际跨界** | **`syscall` 指令 / 软中断** 等 |
| **内核** | `sys_read()` 等 |

#### Unix 设计原则

> **提供机制，而不是策略**（mechanism, not policy）

| 机制 | 策略 |
|------|------|
| 内核提供 **抽象能力**（读 fd、映射内存） | **用户程序决定** 何时读、读多少、怎么用 |

**HFT：** 热路径倾向 **批量 I/O、`mmap`、用户态轮询/DPDK** — 本质是在 **减少机制调用次数**。

→ [02 SysPerf §3.2 syscall 成本](../../02-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [Ch 1 user/kernel 边界](./chapter-01-Linux内核简介.md)

---

### ② 系统调用基础 · Numbers & Naming

#### 内核侧命名与 ABI

| 约定 | 说明 |
|------|------|
| **`asmlinkage`** | 参数 **仅从栈** 取（历史 ABI 约定） |
| **`sys_` 前缀** | 用户 `bar()` → 内核 **`sys_bar()`** |

#### 系统调用号

| 规则 | 说明 |
|------|------|
| 每个 syscall **唯一编号** | 如 x86 `__NR_read` |
| **`sys_call_table[]`** | 内核维护的 **函数指针表** — 下标 = 号 |
| **号一旦分配永不回收** | 保证 **ABI 稳定** |
| 历史 syscall 被移除 | 槽位填 **`sys_ni_syscall()`** — 只返回 **`-ENOSYS`** |

```c
/* 概念示意 */
asmlinkage long sys_read(unsigned int fd, char __user *buf, size_t count);
```

→ 用户态查号：`unistd.h` / `asm/unistd.h` · `strace` 可见实际号

---

### ③ 系统调用处理程序 · Handler & Parameters

用户态 **不能直接执行内核代码** — 必须 **陷入（trap）** 切到内核态。

#### 触发方式（x86 演进）

| 机制 | 说明 |
|------|------|
| **`int $0x80`** | 经典 **软件中断**（32 位时代常见） |
| **`sysenter` / `syscall`** | 更快路径（现代 64 位主流为 **`syscall`**） |

流程概念：

```
用户态 syscall 包装
    │
    ▼
软中断 / syscall 指令
    │
    ▼
异常向量 ──► system_call 入口（架构相关）
    │
    ▼
查 sys_call_table[nr] ──► sys_*(...)
    │
    ▼
返回值 ──► 回到用户态
```

#### x86（书中 32 位约定）

| 寄存器 | 用途 |
|--------|------|
| **`eax`** | **系统调用号** 入 · **返回值** 出 |
| **`ebx, ecx, edx, esi, edi, ebp`** | 参数（按序，个数因调用而异） |

> **x86-64** 上约定不同（如号在 **`rax`**，参数用 **`rdi, rsi, rdx, r10, r8, r9`**）— 思想相同：**寄存器传号与参**。

→ 教学对照：[08-1 Day 20 INT 0x40 API](../../08-system-low-level-hands-on/08-1-30days-os/notes/day-20-API.md)（**稳定号 + 功能号路由**）

---

### ④ 实现与参数验证 · Implementation & Security

#### 设计原则

| 建议 | 反例 |
|------|------|
| **功能单一** | 一个 syscall 干太多事 |
| 避免「万能 syscall」 | 作者批评的 **`ioctl()` 模式** — 一个入口塞无数私有命令 |

#### 参数验证（最关键）

内核 **必须** 假定用户空间 **恶意或错误**：

| 检查 | 内容 |
|------|------|
| 指针合法性 | 是否指向 **合法用户空间** 地址 |
| 访问权限 | 可读 / 可写 / 可执行 |
| 长度与范围 | 缓冲区不会越界到内核 |

#### 绝不要直接解引用用户指针

| 函数 | 方向 |
|------|------|
| **`copy_from_user()`** | 用户 → 内核 |
| **`copy_to_user()`** | 内核 → 用户 |

失败时返回 **`-EFAULT`** 等 — 防止 **内核误访问用户地址导致 oops**。

#### 权能 · Capabilities

特权操作不只靠 UID，还用 **`capable()`** 等检查 **细粒度权能**，例如：

| 权能 | 操作示例 |
|------|----------|
| **`CAP_SYS_REBOOT`** | `reboot()` |
| **`CAP_NET_ADMIN`** | 网络配置 |

→ **Ch 9–10** 锁与并发 · **Ch 7** 中断上下文（不可睡眠）对比

---

### ⑤ 系统调用上下文 · System Call Context

syscall 在 **进程上下文（process context）** 执行：

| 属性 | 含义 |
|------|------|
| **`current`** | 指向 **发起 syscall 的任务**（`task_struct`） |
| **可睡眠** | 缺页、显式 `schedule()`、等锁… |
| **可抢占** | 内核抢占开启时 — 同 **Ch 4** |

因此 syscall 实现必须：

| 要求 | 原因 |
|------|------|
| **可重入（reentrant）** | 同一 `sys_*` 可能被多线程并发进入 |
| **锁保护共享数据** | 防止竞态 |

```
进程 A 线程1 ──read()──► sys_read ──► 可能睡眠（等磁盘）
进程 A 线程2 ──write()──► sys_write ──► 并发 ──► 需锁/无共享或可重入设计
```

**HFT：** 一次 `read`/`send` 尖刺 — 除用户态逻辑外，还要看 **是否阻塞、是否持锁过久、是否触发调度**。

---

### ⑥ 添加系统调用与替代方案

#### 添加新 syscall（步骤概念）

| 步骤 | 动作 |
|------|------|
| 1 | 实现 **`sys_foo()`** |
| 2 | **`sys_call_table` 末尾** 追加一项 |
| 3 | 架构头文件定义 **`__NR_foo`** |
| 4 | 编进内核镜像 |

无 libc 封装时，历史上可用 **`_syscalln()`** 宏系列从用户态直接发起。

#### 为何应极度谨慎

| 事实 | 后果 |
|------|------|
| 主线一旦发布 syscall | 接口 **永久兼容** — 「刻在石头上」 |
| 滥用 | ABI 膨胀、安全面扩大、维护债 |

#### 作者推荐的替代方案

| 方案 | 适用 |
|------|------|
| **字符设备** + `read()` / `write()` | 流式或简单命令 |
| **`sysfs` 属性文件** | 简单配置 / 状态交换 |
| **`ioctl` 于已有设备** | 已有节点上的扩展（仍要克制） |

**HFT 工程：** 生产 rarely 改内核 syscall；调优多用 **已有接口**（`epoll`、`mmap`、`setsockopt`、netlink）或 **内核模块 / 驱动**。

→ 收官：[chapter-20-补丁开发和社区.md](./chapter-20-补丁开发和社区.md) · [01 LFS Ch5 清单](../../05-Linux-Kernel-Development/01_Course_LFS/CHECKLIST.md)

---

### Ch 5 小结

| 问题 | 答案 |
|------|------|
| 用户如何进内核？ | **syscall（+ 异常）** — 常经 **libc 包装** |
| 内核怎么分发？ | **号 → `sys_call_table` → `sys_*`** |
| x86 怎么传号？ | **`eax`**（64 位为 `rax` 等） |
| 安全核心？ | **验证指针 + `copy_*_user` + `capable`** |
| 什么上下文？ | **进程上下文** — 可睡眠、可抢占、要可重入 |
| 能随便加 syscall 吗？ | **否** — 优先 **设备节点 / sysfs** 等 |

---

### 检查单

- [ ] 区分 **libc API** 与 **底层 syscall**
- [ ] 说出 **`copy_from_user` / `copy_to_user`** 为何必须
- [ ] 对比 **syscall 进程上下文** vs **中断上下文**（Ch 7 不可睡眠）
- [ ] 解释 **号不回收** 与 **`sys_ni_syscall`**
- [ ] 能举 HFT **减 syscall** 手段（`mmap`、批量、旁路）

---

## 相关章节

- 上一章：[chapter-04-进程调度.md](./chapter-04-进程调度.md)
- 下一章：[chapter-06-内核数据结构.md](./chapter-06-内核数据结构.md)
- 本模块导读：[README.md](./README.md) · [OUTLINE.md](./OUTLINE.md)
