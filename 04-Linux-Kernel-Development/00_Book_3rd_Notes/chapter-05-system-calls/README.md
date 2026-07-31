# Ch 5 系统调用 · System Calls

> **Linux Kernel Development 3rd** · Robert Love · **选读**

> 本章定位：**用户态 ↔ 内核** 的合法正门 — syscall 号、`sys_call_table`、陷入路径、**参数验证**、进程上下文。  
> 与 [Ch3 §3.8 fd / struct file / inode](../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md) 打通：`open/read/write/fork/dup` 全走 syscall。  
> HFT：**少 syscall、懂延迟从哪来** 的底层一页。

---

## 本节结构

| 节 | 主题 | 带走什么 |
|----|------|----------|
| **① 与内核通信** | 特权级 · libc≠syscall | **机制，非策略** |
| **② 基础** | 号 · `sys_call_table` | 寄存器传参 · 返回值/`errno` |
| **③ 处理程序** | `entry_SYSCALL_64` · 内核栈 | **`do_syscall_64` 分发** |
| **④ 实现与安全** | `__user` · 校验 | **`copy_*_user`** |
| **⑤ 上下文** | 进程上下文 · `current` | 可睡眠 · vs 中断上下文 |
| **⑥ 添加 syscall** | 慎增新号 | **ioctl / netlink / sysfs / eBPF** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 与内核通信 | [notes/section-5.1-与内核通信.md](./notes/section-5.1-与内核通信.md) |
| 系统调用基础 | [notes/section-5.2-系统调用基础.md](./notes/section-5.2-系统调用基础.md) |
| 系统调用处理程序 | [notes/section-5.3-系统调用处理程序.md](./notes/section-5.3-系统调用处理程序.md) |
| 实现与参数验证 | [notes/section-5.4-实现与参数验证.md](./notes/section-5.4-实现与参数验证.md) |
| 系统调用上下文 | [notes/section-5.5-系统调用上下文.md](./notes/section-5.5-系统调用上下文.md) |
| 添加系统调用与替代方案 | [notes/section-5.6-添加系统调用与替代方案.md](./notes/section-5.6-添加系统调用与替代方案.md) |

---

## 串联：把系统调用和 fd 模型打通

完整走一遍 `open()`：

1. 用户态程序调用 libc `open()`
2. libc 准备参数，执行 `syscall`，传入系统调用号
3. CPU 陷入内核，进入 `entry_SYSCALL_64`
4. `do_syscall_64` 找到 `sys_openat`（等）
5. `sys_openat`：
   - 根据路径找到 **inode**；
   - 创建全新 **`struct file`**（打开会话，自带独立 offset）；
   - 在 `current->files`（进程文件表）分配一个空闲 **fd**；
   - **fd → struct file** 建立映射；
6. 返回 fd 数字给用户态

场景对比（复习 §3.8）：

| 场景 | 结果 |
|------|------|
| **两次 open 同一文件** | 两次 `sys_openat` → **两个** `struct file`、两个 fd，offset **互不干扰** |
| **dup(fd)** | `sys_dup` → 新 fd 指向 **同一** `struct file`，**共享** offset |
| **fork()** | 子进程复制 fd 表 → 相同 fd 指向 **同一** `struct file`，**共享** offset |

示例代码：  
[`dual_open_fd_offset_demo.c`](../chapter-03-process-management/code/dual_open_fd_offset_demo.c) ·  
[`fork_fd_offset_demo.c`](../chapter-03-process-management/code/fork_fd_offset_demo.c)

---

## 思考题（答案要点）

1. **为什么中断上下文里不能调用会走 `read`/`write` 那类路径？**  
   那些路径常在 **进程上下文** 里可能 **睡眠**（等磁盘、锁、页）。中断上下文 **禁止休眠** — 一睡可能死锁或搞坏调度假设。中断里应只做短、不可睡眠的工作，其余推到进程上下文 / workqueue。

2. **若在 `sys_read` 里直接访问用户 `buf`，不用 `copy_to_user`？**  
   用户指针未经验证：可能非法、不可写、或触发复杂缺页路径。轻则返回错误处理不当，重则 **内核 Oops / 安全漏洞**。必须用 `copy_*_user`（或同类安全接口）。

3. **`close(fd)` 立刻删除 inode 吗？何时真正销毁？**  
   **不会立刻删 inode。** `close` 先释放该进程里这个 fd 槽；`struct file` 引用计数减一，到 **0** 才释放打开会话；inode（及磁盘上的文件数据）还要看 **链接数 / 其他打开者** — 无打开且 unlink 后等条件满足才真正销毁。详见 §3.8。

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 用户如何进内核？ | **syscall（+ 异常）** — 常经 **libc 包装** |
| 内核怎么分发？ | **号 → `sys_call_table` → `sys_*`** |
| x86_64 怎么传号？ | **`rax`**（书中 32 位多为 `eax`） |
| 安全核心？ | **验证指针 + `copy_*_user` + `capable`** |
| 什么上下文？ | **进程上下文** — 可睡眠、可抢占、要可重入 |
| 能随便加 syscall 吗？ | **否** — 优先 **ioctl / netlink / sysfs / eBPF** |

---

## 本章学习目标 · 自检

- [ ] 区分 **libc API** 与 **底层 syscall**
- [ ] 说出 **`copy_from_user` / `copy_to_user`** 为何必须
- [ ] 对比 **syscall 进程上下文** vs **中断上下文**（Ch 7 不可睡眠）
- [ ] 解释 **号不回收** 与 **`sys_ni_syscall`**
- [ ] 能把 **`open` → fd → struct file → inode`** 走通
- [ ] 能举 HFT **减 syscall** 手段（`mmap`、批量、旁路）

---

## 相关章节

- 上一章：[../chapter-04-process-scheduling/](../chapter-04-process-scheduling/)
- 下一章：[../chapter-06-kernel-data-structures/](../chapter-06-kernel-data-structures/)
- FD 模型：[../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md](../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md)
- 全书导读：[../README.md](../README.md) · [../OUTLINE.md](../OUTLINE.md)
