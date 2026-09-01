## ⑤ 神奇的 SysRq 键 · Magic SysRq

**`CONFIG_MAGIC_SYSRQ`** — 系统 **死锁/假死** 时仍可向内核发 **底层命令**。

| x86 组合 | **`Alt + SysRq(PrtSc) + 字母`** |

| 命令 | 含义 |
|------|------|
| **`SysRq-s`** | **sync** — 脏缓冲写盘 |
| **`SysRq-u`** | **umount** — 卸载 FS（remount 只读） |
| **`SysRq-b`** | **reboot** — 立即重启（**未 sync 会丢数据**） |
| `SysRq-c` | **crash** — 故意 panic（**配 kdump 抓 vmcore** 的标准触发器） |
| `SysRq-f` | 调用 OOM killer |
| `SysRq-m` | 打印内存信息 |
| `SysRq-p` | 打印寄存器 |
| `SysRq-t` | 打印所有任务栈 |
| `SysRq-w` | 打印阻塞（D/不可中断）任务 |

| 救命序列（经典） | **`s` → `u` → `b`** — 尽量安全重启 |
|------------------|--------------------------------------|

```
系统无响应但 SysRq 仍通
    ▼
s（落盘）→ u（卸盘）→ b（重启）
```

#### 触发的三条通道

| 通道 | 写法 |
|------|------|
| 键盘 | `Alt + SysRq(PrtSc) + 字母` |
| 串口控制台 | `BREAK` 序列（串口服务器场景——机房无显示器） |
| **命令行回显** | `echo c > /proc/sysrq-trigger`（sshd 还活着时） |

开关：`/proc/sys/kernel/sysrq`（0 全关 / 1 全开 / 位掩码挑功能）——**发行版常默认只留 sync/reboot 几位**。

#### 它"神奇"在哪——以及边界在哪

| 层 | 是否绕过 |
|----|----------|
| 用户空间、调度器、运行队列 | ✓（不依赖任何进程活着） |
| 文件系统、网络栈 | ✓ |
| **键盘控制器硬件中断（IRQ）** | ✗ **依赖**——本质仍是普通中断处理 |

> SysRq 由键盘控制器的**中断处理程序**在内核最底层分发（串口版走串口驱动）。所以它不需要调度器、不需要进程，但它**不是 NMI**：如果 CPU 关中断（`spin_lock_irqsave` 死循环）或中断控制器/键盘驱动本身被锁死，SysRq 同样进不来。真正中断全关的硬死锁只剩：NMI watchdog（检测并报告 soft lockup）、IPMI/BMC 带外通道、硬件复位。

**HFT 实盘：** 慎用；Prefer **优雅下线** — 懂即可。

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 系统完全死锁（键盘无响应）时，SysRq 还能用吗？

<details><summary>答案</summary>

**视死锁类型而定**。SysRq 不依赖调度器/进程/文件系统（这些全挂也能通），由键盘控制器的**中断处理程序**在内核底层分发——调度死锁、D 状态风暴、用户空间全冻结时依然有效。但它**不是 NMI**：CPU 因 `spin_lock_irqsave` 死循环而**关中断**、或键盘驱动/中断控制器本身被锁死时，SysRq 一样进不来。中断全关的硬死锁只剩 NMI watchdog（检测报告）、IPMI/BMC 带外通道、硬件复位三条路。HFT 系统建议配 kdump，`SysRq-c` 触发 panic 自动保存 vmcore。

</details>

**Q2.** 为什么 SysRq+s → u → b 的顺序很重要？

<details><summary>答案</summary>

s(sync) 把脏页写回磁盘 → u(umount) 卸载文件系统标记干净 → b(reboot) 立即重启。如果跳过 s 直接 b，脏页丢失（文件系统损坏）。如果跳过 u 直接 b，文件系统标记为"未干净卸载"，重启后 fsck 检查。s→u→b 是最小化数据丢失的安全重启序列。生产环境也可以用 `echo s > /proc/sysrq-trigger` 通过命令行触发。

</details>

**Q3.** 为什么不少发行版默认把 `/proc/sys/kernel/sysrq` 设为 176 而不是 1？这个数字是什么意思？

<details><summary>答案</summary>

sysrq 开关是**位掩码**不是布尔值：每一位对应一组功能的允许/禁止。176 = 0b10110000 = 允许 sync(16) + remount(32) + 信号/调试(128)，而**不含** reboot(2)、crash(4) 等更危险的动作——防止误触发或被本地低权用户借 `/proc/sysrq-trigger` 重启机器（该文件写入权限有限制，但纵深防御）。要全开得显式 `echo 1 > /proc/sys/kernel/sysrq`；HFT 机器若依赖 SysRq-c 触发 kdump，必须确认 crash 位（4）在掩码里。

</details>

</details>

---
