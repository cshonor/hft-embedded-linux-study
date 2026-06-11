# a03 内核架构总览 · Kernel and Userland Separation

> **中英字幕视频教程** · Complete Kernel Guide · **精读**

> 承接 [a01 Unix DNA / 简单性与模块化](./episode-a01-Unix设计基因.md) — 本讲是 Unix/Linux **最核心**的架构分界。

---

## 幻灯片要点

### 1. 严格分离内核与用户态

> *Unix strongly separated kernel responsibilities from userland applications.*

| | **内核态（Kernel）** | **用户态（Userland）** |
|---|---------------------|------------------------|
| **职责** | 进程、内存、硬件抽象、I/O 调度 | 应用逻辑、工具、服务 |
| **权限** | 最高特权（Ring 0） | 受限（Ring 3） |
| **访问硬件** | 直接 | **不可以** — 必须经系统调用 |
| **访问受保护内存** | 全部（含用户空间拷贝） | 仅本进程地址空间 |

### 2. 降复杂度、提稳定与安全

> *This reduced kernel complexity and improved stability and security.*

- 内核代码**精简、职责单一** → 更易维护
- **应用崩溃 ≠ 内核崩溃**（多数情况）
- 恶意/ buggy 用户程序**不能直接**改内核数据结构或硬件寄存器

→ 内核编程（[02 e2 LKM](../../02_Course_Kernel_7Lectures/episode-e02-Linux内核模块.md)）运行在**内核态**，一行 bug 可能 Oops — 必须格外谨慎。

### 3. 用户态提供丰富功能，内核不臃肿

> *Userland tools provide rich functionality without bloating the kernel.*

- 文本处理、网络服务、GUI — 均在用户态
- 可**独立安装/升级**用户态工具，**无需重编译内核**
- [LFS p6–p9](../../01_Course_LFS/) 装的全是 userland GNU 工具；[p11–p12](../../01_Course_LFS/episode-p11-内核编译上.md) 才是内核

### 4. Linux 继承 → 用户态创新

> *Linux inherited this model, facilitating rapid user-space innovation.*

- 新工具、新发行版、新语言运行时 — 大多只需 userland 开发
- 生态「百花齐放」的根因之一

---

## 通俗类比：银行与客户

| | 内核（银行） | 用户态（客户） |
|---|-------------|---------------|
| 角色 | 管金库（内存）、办交易（I/O）、控硬件 | 外面办事：ATM、转账、业务 |
| 接口 | 固定窗口 = **系统调用** | 不能闯进金库改账本 |
| 结果 | 银行稳定安全 | 客户活动灵活多样 |

---

## 交互流程：用户态 → 系统调用 → 内核态

```
┌─────────────────────────────────────────────────────────┐
│  用户态 (Userland)                                       │
│  ┌──────────┐    ┌──────────┐    ┌──────────────────┐   │
│  │ 你的程序  │───▶│ glibc    │───▶│ syscall 指令      │   │
│  │ trading  │    │ read()   │    │ (syscall/sysenter)│   │
│  │ engine   │    │ write()  │    └─────────┬────────┘   │
│  └──────────┘    │ socket() │              │            │
│                  └──────────┘              ▼            │
└────────────────────────────────────────────│────────────┘
                                             │ 特权级切换
┌────────────────────────────────────────────▼────────────┐
│  内核态 (Kernel)                                          │
│  sys_call_table → 子系统分发                               │
│    · VFS (read/write 文件)                                │
│    · 网络栈 (socket → TCP/UDP → sk_buff → 驱动)          │
│    · 内存管理 (mmap, brk, page fault)                     │
│    · 进程调度 (fork, clone, sched)                        │
│  ┌─────────────┐                                          │
│  │ LKM 模块    │  ← 02 内核编程：也运行在此，同内核权限     │
│  └─────────────┘                                          │
└────────────────────────────────────────────│────────────┘
                                             ▼
                                        硬件 (CPU / MMU / 网卡)
```

**一次 `read(fd, buf, n)` 的简化路径：**

1. 用户程序调用 `read()`（libc 包装）
2. 触发系统调用进入内核
3. 内核 VFS 层 → 具体文件系统 / socket / 设备驱动
4. 数据拷贝到用户 `buf`（`copy_to_user`）
5. 返回用户态，程序继续执行

---

## 与三门课 / HFT 的对应

| 学习线 | 在这条边界上的位置 |
|--------|-------------------|
| **LFS** | 根文件系统里全是 userland 二进制；它们**只**能 syscall 进内核 |
| **02 内核编程** | LKM = 内核态代码；测试程序 = 用户态，通过 `/dev` 或 syscall 交互 |
| **05 UNP / CSAPP Ch11** | `socket`、`epoll` 是 **userland API**；协议栈在 kernel |
| **06 Rosen** | 画清 syscall 之下 sk_buff、NAPI 怎么走 |
| **12 DPDK** | **旁路**部分内核网络路径，但仍依赖内核做初始化/绑定；不是「完全不要内核」 |

| HFT 场景 | 含义 |
|----------|------|
| 策略进程 | 典型 userland；绑核、`mlock`、大页 — 仍是 syscall |
| 内核模块 | 极少 HFT 自写；理解即可，bug 代价极高 |
| DPDK 收包 | userland 轮询 NIC；理解「哪些仍靠内核配置」 |

→ 三门课总对照：[CROSS-COURSE.md](./CROSS-COURSE.md)  
→ 内核栈 vs DPDK：[CROSS-MODULE-GUIDE.md](../../CROSS-MODULE-GUIDE.md#二内核网络栈-vs-用户态旁路)

---

## 相关

- 上一讲：[episode-a02-宏内核与微内核.md](./episode-a02-宏内核与微内核.md)
- 下一讲：[episode-a04-固件与引导流程.md](./episode-a04-固件与引导流程.md)
- 前置：[episode-a01-Unix设计基因.md](./episode-a01-Unix设计基因.md)
- 书本：LKD [Ch 5 系统调用](../../00_Book_3rd_Notes/chapter-05-系统调用.md) · [Ch 1 简介](../../00_Book_3rd_Notes/chapter-01-Linux内核简介.md)
- 三门课对照：[CROSS-COURSE.md](./CROSS-COURSE.md)
- 总目录：[OUTLINE.md](./OUTLINE.md)
