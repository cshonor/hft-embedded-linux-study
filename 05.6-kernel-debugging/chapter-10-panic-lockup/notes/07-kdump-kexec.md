# Kdump / Kexec 崩溃转储

> 🔴 精读

## 概念详解

### Kdump 原理

```
正常系统:
  ┌───────────────────────────┐
  │  Production Kernel        │
  │  (运行中)                  │
  │  预留: crashkernel=128M    │
  └───────────────────────────┘

Panic 时:
  ┌───────────────────────────┐
  │  Production Kernel (死)    │
  │  ┌─────────────────────┐  │
  │  │ Crash Kernel (kexec)│  │ ← 在预留内存中启动
  │  │ 收集生产内核的内存   │  │
  │  │ 生成 vmcore dump    │  │
  │  └─────────────────────┘  │
  └───────────────────────────┘

关键: crash kernel 是全新的内核，不受生产内核损坏状态影响
```

### Kexec 工作原理

```
1. 正常运行时: kexec 加载 crash kernel 到预留内存
   kexec -p /boot/vmlinux --append="root=... irqpoll maxcpus=1"

2. Panic 时: 生产内核调用 kexec 跳转
   → 不经过 BIOS 重启
   → 直接跳转到 crash kernel 入口
   → 生产内核的内存内容保留

3. Crash kernel 启动:
   → 以最小配置启动 (1 CPU, 无中断)
   → 通过 /proc/vmcore 暴露生产内核内存
   → 收集并保存 vmcore dump
   → 完成后重启
```

### 配置 Kdump

```bash
# 1. 内核配置
CONFIG_KEXEC=y
CONFIG_CRASH_DUMP=y
CONFIG_PROC_VMCORE=y

# 2. 预留内存 (boot 参数)
# crashkernel=128M  — 预留 128MB 给 crash kernel
# crashkernel=256M  — 更多内存 (大型系统)
# crashkernel=128M@512M  — 指定位置

# 3. 安装 kdump 工具
sudo apt install kdump-tools

# 4. 配置
sudo dpkg-reconfigure kdump-tools
# 选择 "use kdump"

# 5. 验证
cat /proc/sys/kernel/crashkernel_addr  # crash kernel 加载地址
kexec -p /boot/vmlinux --append="root=... irqpoll maxcpus=1"

# 6. 测试触发
echo c > /proc/sysrq-trigger  # 手动触发 panic (测试用!)
```

### 分析 vmcore

```bash
# crash 工具分析 vmcore
crash /usr/lib/debug/vmlinux /var/crash/20260810120000/vmcore

# 常用 crash 命令
crash> bt                # 崩溃线程的栈回溯
crash> ps                # 所有进程
crash> log               # dmesg 日志
crash> kmem -s           # slab 信息
crash> struct task_struct ffff000012345678  # 查看特定结构体
crash> foreach bt        # 所有进程的栈
crash> sym schedule      # 查看函数地址
crash> mod               # 已加载模块
crash> dev               # 设备信息
```

### crash 工具常用命令

| 命令 | 功能 | 用途 |
|------|------|------|
| `bt` | 栈回溯 | 查看崩溃线程的调用链 |
| `ps` | 进程列表 | 查看所有进程状态 |
| `log` | 内核日志 | 查看 dmesg 输出 |
| `kmem -s` | slab 信息 | 检查内存分配状态 |
| `kmem -p` | 页表 | 查看物理内存映射 |
| `foreach bt` | 所有栈 | 查看所有进程的栈 |
| `struct` | 结构体 | 查看特定数据结构 |
| `sym` | 符号查找 | 地址↔符号映射 |
| `mod` | 模块 | 已加载模块列表 |
| `dev` | 设备 | 设备信息 |
| `search` | 内存搜索 | 搜索特定值 |

### 树莓派上的 Kdump

```bash
# 树莓派 5 Kdump 配置
# /boot/cmdline.txt
console=serial0,115200 crashkernel=128M

# 树莓派 5 (4GB) 可以预留 128MB 给 crash kernel
# 剩余 ~3.8GB 可用

# 注意: 树莓派需要编译 crash kernel
# 通常是同一个内核镜像，以不同参数启动
```

### HFT 关联应用

```bash
# HFT 生产环境 Kdump 配置
# /etc/sysctl.d/99-hft.conf
kernel.panic_on_oops = 1
kernel.panic = 5  # 5 秒后重启

# Kdump 配置
# crashkernel=256M  # 预留 256MB
# kdump-tools 自动收集 vmcore 并重启

# 自动收集流程:
# 1. Panic 发生
# 2. Kexec 跳转到 crash kernel
# 3. Crash kernel 收集 vmcore 到 /var/crash/
# 4. 收集完成后自动重启
# 5. 重启后 kdump-tools 自动通知运维

# HFT 价值: 崩溃后自动重启 + 保留完整内存镜像供分析
```

### Kdump 的内存开销

| 配置 | 预留内存 | 适用场景 |
|------|---------|---------|
| `crashkernel=64M` | 64MB | 小型系统（树莓派 3） |
| `crashkernel=128M` | 128MB | 标准系统（树莓派 5） |
| `crashkernel=256M` | 256MB | 大型系统（服务器） |
| `crashkernel=512M` | 512MB | 超大内存系统 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Kdump 如何在 panic 后收集崩溃信息？

> Kdump 预先加载一个 crash kernel（kexec）。Panic 时，生产内核调用 kexec 跳转到 crash kernel（在预留内存中启动）。Crash kernel 启动后，生产内核的内存仍保留，crash kernel 通过 /proc/vmcore 将其导出为 dump 文件。

**Q2:** kdump 的工作原理是什么？

> 正常内核运行时预加载 crash kernel 到保留内存。panic 发生时，kexec 跳转到 crash kernel（不经过 BIOS 重启）。crash kernel 以最小配置启动，通过 /proc/vmcore 暴露原内核的物理内存镜像。用 crash 工具分析 vmcore。

**Q3:** HFT 系统是否应该启用 kdump？

> 应该启用。HFT 崩溃时需要分析 root cause，kdump 保存完整内存镜像。crash kernel 保留内存（通常 128MB-256MB）对 HFT 影响小。建议生产环境配置 kdump + 自动收集 vmcore + 自动重启。

**Q4:** crash 工具的 `bt` 和 `foreach bt` 有什么区别？

> `bt` 只显示当前崩溃线程的栈回溯。`foreach bt` 显示所有进程/线程的栈回溯。后者帮助分析崩溃时全局状态——如哪些线程在等锁、哪些在中断处理中。

**Q5:** 为什么 crash kernel 用 `maxcpus=1` 参数？

> 限制 crash kernel 只使用 1 个 CPU。原因：(1) 多 CPU 需要更多内存；(2) 多 CPU 可能导致竞态条件（生产内核停止时的状态不一致）；(3) 单 CPU 足够收集 vmcore。`maxcpus=1` 确保 crash kernel 简单可靠地启动。

</details>

## 交叉引用

- [05.6 ch10 Panic 触发与处理](../../chapter-10-panic-lockup/notes/01-panic-causes.md)
- [05.6 ch10 自定义 Panic Handler](../../chapter-10-panic-lockup/notes/06-custom-panic-handler.md)
- [05.6 ch07 Oops vs Panic](../../chapter-07-oops/notes/01-oops-vs-panic.md)
