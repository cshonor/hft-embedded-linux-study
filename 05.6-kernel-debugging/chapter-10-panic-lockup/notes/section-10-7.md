# 10.7 Kdump / Kexec 崩溃转储

> 🔴 精读

## 本节要点

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
```

### 配置 Kdump

```bash
# 1. 内核配置
CONFIG_KEXEC=y
CONFIG_CRASH_DUMP=y
CONFIG_PROC_VMCORE=y

# 2. 预留内存 (boot 参数)
# crashkernel=128M  — 预留 128MB 给 crash kernel

# 3. 安装 kdump 工具
sudo apt install kdump-tools

# 4. 配置
sudo dpkg-reconfigure kdump-tools
# 选择 "use kdump"

# 5. 验证
cat /proc/sys/kernel/crashkernel_addr  # crash kernel 加载地址
kexec -p /boot/vmlinux  --append="root=... irqpoll maxcpus=1"
# 或通过 kdump-tools 自动配置

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
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Kdump 如何在 panic 后收集崩溃信息？

> Kdump 预先加载一个 crash kernel（kexec）。Panic 时，生产内核调用 kexec 跳转到 crash kernel（在预留内存中启动）。Crash kernel 启动后，生产内核的内存仍保留，crash kernel 通过 /proc/vmcore 将其导出为 dump 文件。因为 crash kernel 是全新的内核，不受生产内核损坏状态影响。

</details>
