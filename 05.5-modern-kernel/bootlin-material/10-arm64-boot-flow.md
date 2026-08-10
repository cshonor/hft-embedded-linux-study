# Bootlin: ARM64 内核启动流程

> **来源:** [Bootlin ARM64 Training](https://bootlin.com/docs/arm/)
> **主题:** ARM64 内核启动链 (head.S → start_kernel → userspace)
> **对标旧书:** ULK3 Ch2 (x86 启动) / 嵌入式C Ch04

---

## 讲义要点

### ARM64 内核启动阶段

```
┌──────────────────────────────────────────────────────┐
│ 1. ROM Bootloader / U-Boot                            │
│    加载 kernel Image + dtb 到内存，跳转               │
├──────────────────────────────────────────────────────┤
│ 2. arch/arm64/kernel/head.S (汇编头)                 │
│    设置初始页表、开启 MMU、跳转 C 代码                │
├──────────────────────────────────────────────────────┤
│ 3. start_kernel() (init/main.c)                      │
│    初始化所有子系统、调度器、中断、驱动               │
├──────────────────────────────────────────────────────┤
│ 4. rest_init() → kernel_init()                        │
│    挂载 rootfs、执行 /sbin/init                       │
├──────────────────────────────────────────────────────┤
│ 5. 用户空间 (systemd / init)                          │
│    启动服务、进入用户会话                              │
└──────────────────────────────────────────────────────┘
```

### 阶段 1: U-Boot → 内核入口

```
U-Boot 将内核加载到内存后跳转:
  x0 = dtb 地址 (设备树在内存中的位置)
  x1-x3 = 0
  x4 = 0 (或预留)
  PC = 内核入口地址 (Image 头中的跳转指令)

# ARM64 Image 头 (arch/arm64/kernel/head.S)
# 前两条指令是无操作跳转，后续是头信息
```

### 阶段 2: head.S (汇编初始化)

```asm
// arch/arm64/kernel/head.S
__primary_entry:
    bl      preserve_boot_args      // 保存 x0-x3 (dtb 地址等)
    bl      el2_setup               // 检查异常等级，降级到 EL1
    bl      set_cpu_boot_mode_flag
    bl      __create_page_tables    // 创建初始页表 (恒等映射 + 内核映射)
    bl      __cpu_setup             // 配置 SCTLR_EL1 (MMU/cache 控制寄存器)
    bl      __primary_switch        // 开启 MMU，跳转到 C 代码

__primary_switch:
    bl      __enable_mmu            // 设置 TTBR0/TTBR1，开启 MMU
    ldr     x8, =__primary_switched
    br      x8                      // 跳转到 C 代码
```

关键步骤：
1. **EL2 → EL1 降级**：如果内核在 EL2 启动（Hypervisor 模式），先降级到 EL1
2. **创建初始页表**：建立内核镜像的恒等映射（物理地址 = 虚拟地址）+ 内核虚拟地址映射
3. **开启 MMU**：设置 `TTBR0_EL1` / `TTBR1_EL1` 页表基址寄存器，设置 `SCTLR_EL1.M = 1`

### 阶段 3: start_kernel() (C 代码)

```c
// init/main.c
asmlinkage __visible void __init start_kernel(void) {
    setup_arch(&command_line);         // 架构初始化 (解析 dtb, 内存布局)
    mm_init();                         // 内存管理初始化 (buddy, slab)
    sched_init();                      // 调度器初始化
    early_irq_init();                  // 中断子系统初始化
    init_IRQ();
    time_init();                       // 时钟初始化 (hrtimer, clocksource)
    console_init();                    // 控制台初始化 (printk 可用)
    // ... 大量子系统初始化 ...
    rest_init();                       // 创建 init 线程 (PID 1)
}

static noinline void __init_refok rest_init(void) {
    kernel_thread(kernel_init, NULL, CLONE_FS);  // PID 1 = init
    cpu_startup_entry(CPUHP_AP_ONLINE_IDLE);      // PID 0 = idle
}

static int __ref kernel_init(void *unused) {
    kernel_init_freeable();
    // 挂载 rootfs
    if (ramdisk_execute_command) {
        run_init_process(ramdisk_execute_command);
    }
    // 执行 /sbin/init, /etc/init, /bin/init, /bin/sh
    if (!try_to_run_init_process("/sbin/init") ||
        !try_to_run_init_process("/etc/init") ||
        !try_to_run_init_process("/bin/init") ||
        !try_to_run_init_process("/bin/sh"))
        panic("No init found");
}
```

### 启动参数传递

```
U-Boot → 内核:
  x0 = dtb 物理地址
  bootargs (在 dtb 的 /chosen 节点中)

内核解析 bootargs:
  console=serial0,115200    → 控制台设备
  root=/dev/mmcblk0p2       → rootfs 设备
  rootwait                  → 等待 rootfs 设备就绪
  rw                        → rootfs 可读写
  init=/sbin/init           → init 程序路径
  isolcpus=2-3              → 隔离 CPU 2-3
```

---

## 动手实验

```bash
# 1. 查看内核启动日志
dmesg | head -50
# [    0.000000] Booting Linux on physical CPU 0x0000000000 [0x410fd0b1]
# [    0.000000] Linux version 6.1.63-v8+ ...
# [    0.000000] Machine model: Raspberry Pi 5 Model B Rev 1.0
# [    0.000000] Memory: 3865392K/4194304K available

# 2. 查看启动参数
cat /proc/cmdline
# console=serial0,115200 root=/dev/mmcblk0p2 rootwait ...

# 3. 查看启动耗时分析
systemd-analyze
systemd-analyze blame | head -20
systemd-analyze critical-chain

# 4. 查看内核初始化时间线 (需要 CONFIG_PRINTK_TIME=y)
dmesg | awk '{print $1}' | sort -n | tail -1
# 最后一条 dmesg 的时间戳 = 内核初始化耗时

# 5. 查看 init 进程
ps -p 1 -o pid,comm,args
# PID TTY STAT TIME COMMAND
#   1 ?   Ss   0:05 /sbin/init  (或 /lib/systemd/systemd)

# 6. 使用 ftrace 追踪启动过程
# 在内核 cmdline 中添加: trace_event=initcall:* ftrace=function
# 启动后:
cat /sys/kernel/debug/tracing/trace | grep initcall | head -30
```

---

## 与旧书差异

| ULK3 (x86) | Bootlin ARM64 |
|------------|---------------|
| 16 位 real mode → 32 位保护模式 | EL2 → EL1 直接降级 |
| BIOS → bootloader → setup.S → head.S | U-Boot → head.S (无 real mode) |
| `cr3` 页表基址 | `TTBR0_EL1` / `TTBR1_EL1` |
| `e820` 内存映射 | DTB 中的 `/memory` 节点 |
| ACPI 硬件描述 | Device Tree |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** ARM64 内核启动时为什么要先从 EL2 降级到 EL1？

> Linux 内核运行在 EL1 (内核态)。如果系统在 EL2 启动（Hypervisor 模式），`el2_setup()` 会配置虚拟化扩展并降级到 EL1。如果需要 KVM 虚拟化，EL2 的状态会被保存供 Hypervisor 使用。不降级直接运行在 EL2 会影响某些系统寄存器的访问权限。

**Q2:** `head.S` 中创建的"恒等映射"是什么？为什么需要？

> 恒等映射 (identity mapping) 是物理地址 = 虚拟地址的映射。开启 MMU 的瞬间，PC 指向的物理地址必须同时有虚拟地址映射，否则 CPU 取下一条指令时会页错误。恒等映射确保 MMU 开启后当前执行的代码继续可用，直到跳转到内核虚拟地址空间的代码。

**Q3:** 内核如何知道 rootfs 在哪个设备上？

> 通过 bootargs 中的 `root=` 参数。U-Boot 将 bootargs 放在设备树的 `/chosen` 节点中，内核解析设备树时提取 bootargs，在 `kernel_init()` 中挂载 rootfs。如果指定了 `rootwait`，内核会等待 rootfs 设备就绪（SD 卡初始化完成）后再挂载。

</details>
