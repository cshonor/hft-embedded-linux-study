# start_kernel() C 代码初始化与用户空间启动

> 来源: Bootlin ARM64 Training
> 对标旧书: ULK3 Ch2 (init/main.c 已大幅变化)

---

## 阶段 3: start_kernel()

```c
// init/main.c
asmlinkage __visible void __init start_kernel(void) {
    // === 架构初始化 ===
    setup_arch(&command_line);         // 解析 dtb, 内存布局, bootargs
    parse_args("early", command_line, ...);  // 早期参数解析

    // === 内存管理 ===
    mm_init();                         // buddy + slab/slub + vmalloc
    mem_init();                        // 内存 zone 初始化
    kmem_cache_init();                 // SLUB 分配器初始化

    // === 调度器 ===
    sched_init();                      // 调度器初始化 (runqueue, idle thread)

    // === 中断与时钟 ===
    early_irq_init();                  // 中断描述符表初始化
    init_IRQ();                        // 平台中断初始化 (GIC)
    tick_init();                       // clockevents 框架
    time_init();                       // hrtimer, clocksource

    // === 控制台 ===
    console_init();                    // printk 可用 (之前只有 earlycon)

    // === 其他子系统 ===
    vfs_caches_init();                 // VFS 缓存 (dentry, inode)
    buffer_init();                     // buffer head 缓存
    signals_init();                    // 信号处理
    page_writeback_init();             // 页回写
    proc_root_init();                  // /proc 文件系统
    cgroup_init();                     // cgroup v2

    // === 最后阶段 ===
    rest_init();                       // 创建 init 线程 (PID 1)
}
```

### 关键初始化顺序

```
start_kernel()
├── setup_arch()          ← 解析 DTB, 内存布局
├── mm_init()             ← buddy/slab 可用
├── sched_init()          ← 调度器可用
├── early_irq_init()      ← 中断可用
├── time_init()           ← 时钟可用
├── console_init()        ← printk 全功能可用
├── ...大量子系统...
└── rest_init()
    ├── kernel_thread(kernel_init)  ← PID 1 (init)
    └── cpu_startup_entry()          ← PID 0 (idle)
```

---

## 阶段 4: rest_init() → init 线程

```c
static noinline void __init_refok rest_init(void) {
    // 创建 init 线程 (PID 1)
    struct task_struct *tsk;
    tsk = kernel_thread(kernel_init, NULL, CLONE_FS);

    // 当前 CPU 进入 idle 循环 (PID 0)
    cpu_startup_entry(CPUHP_AP_ONLINE_IDLE);
}

static int __ref kernel_init(void *unused) {
    kernel_init_freeable();  // 做更多初始化 (驱动加载等)

    // 尝试挂载 rootfs
    if (ramdisk_execute_command) {
        if (!run_init_process(ramdisk_execute_command))
            return 0;
    }

    // 查找并执行 init 程序
    if (!try_to_run_init_process("/sbin/init") ||
        !try_to_run_init_process("/etc/init") ||
        !try_to_run_init_process("/bin/init") ||
        !try_to_run_init_process("/bin/sh"))
        panic("No init found.  Try passing init= option to kernel.");
}
```

### 驱动加载: do_initcalls()

```c
// kernel_init_freeable() 中调用 do_initcalls()
// 按级别顺序执行 initcall 函数

static void __init do_initcalls(void) {
    // level 0: pure_initcall (最早, 不依赖任何子系统)
    // level 1: core_initcall (核心子系统)
    // level 2: postcore_initcall
    // level 3: arch_initcall (架构特定驱动)
    // level 4: subsys_initcall (子系统)
    // level 5: fs_initcall (文件系统)
    // level 6: device_initcall (常规驱动, 大多数)
    // level 7: late_initcall (最晚)
}
```

```bash
# 查看驱动加载顺序和耗时
dmesg | grep "initcall"
# [    0.123456] initcall bcm2835_gpio_init+0x0/0x10 returned 0 after 120 usecs
# [    0.123789] initcall gic_init+0x0/0x20 returned 0 after 350 usecs
```

---

## 启动参数传递

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
  isolcpus=2-3              → 隔离 CPU 2-3 (HFT)
  nohz_full=2-3             → 无滴带 (HFT)
  rcu_nocbs=2-3             → RCU 回调卸载 (HFT)
```

```bash
# 查看当前启动参数
cat /proc/cmdline

# 在 U-Boot 中设置 bootargs
# U-Boot> setenv bootargs "console=serial0,115200 root=/dev/mmcblk0p2 rootwait isolcpus=2-3 nohz_full=2-3"
# U-Boot> saveenv
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

# 2. 查看启动耗时分析
systemd-analyze
systemd-analyze blame | head -20
systemd-analyze critical-chain

# 3. 查看 initcall 耗时
dmesg | grep "initcall" | sort -t' ' -k2

# 4. 使用 ftrace 追踪启动过程
# 在内核 cmdline 中添加: trace_event=initcall:* ftrace=function
# 启动后:
cat /sys/kernel/debug/tracing/trace | grep initcall | head -30

# 5. 查看 init 进程
ps -p 1 -o pid,comm,args
#   1 ?   Ss   0:05 /sbin/init  (或 /lib/systemd/systemd)
```

---

## HFT 关联

| 启动阶段 | HFT 关注 |
|---------|---------|
| bootargs | isolcpus/nohz_full/rcu_nocbs 参数 |
| initcall 顺序 | 网卡驱动加载时机 (影响网络就绪时间) |
| 启动耗时 | 交易系统冷启动时间 |
| console_init | 日志输出时机 (earlycon vs console) |

> **HFT 实践：** 交易系统冷启动需要最小化启动时间。通过 `systemd-analyze blame` 找到耗时服务，禁用不必要的 initcall。isolcpus 参数在 bootargs 中设置，确保交易线程绑定的 CPU 从启动起就不被普通进程调度。

---

## 自测题

<details>
<summary>Q1: start_kernel() 中为什么 sched_init() 在 init_IRQ() 之前？</summary>

调度器需要先初始化 runqueue 和 idle thread，因为中断处理可能触发调度（如时钟中断触发时间片切换）。如果中断先初始化而调度器没准备好，中断处理中调用 schedule() 会崩溃。但时钟初始化（time_init）需要在中断之后，因为 clockevent 设备注册需要中断子系统。
</details>

<details>
<summary>Q2: 内核如何知道 rootfs 在哪个设备上？</summary>

通过 bootargs 中的 `root=` 参数。U-Boot 将 bootargs 放在设备树的 `/chosen` 节点中，内核解析设备树时提取 bootargs，在 kernel_init() 中挂载 rootfs。如果指定了 `rootwait`，内核会等待 rootfs 设备就绪（SD 卡初始化完成）后再挂载。
</details>

<details>
<summary>Q3: isolcpus=2-3 参数在启动流程的哪个阶段生效？</summary>

在 setup_arch() → smp_init() 阶段解析。内核构建 CPU 调度域时，将 isolcpus 指定的 CPU 从默认调度域中排除，不参与普通进程的负载均衡。但这些 CPU 仍然在线（online），可以通过 sched_setaffinity 显式绑定进程到这些 CPU。HFT 交易线程在用户空间用 sched_setaffinity 绑定到隔离核。
</details>

<details>
<summary>Q4: PID 0 和 PID 1 分别是什么？为什么 PID 0 不退出？</summary>

PID 0 是 idle 线程（每个 CPU 一个），在 rest_init() 中通过 cpu_startup_entry() 进入 idle 循环。当没有可运行的进程时，CPU 执行 idle 线程（WFI 指令让 CPU 进入低功耗）。PID 0 永远不退出——它是调度器的"最后手段"。PID 1 是 init/systemd 进程，负责启动所有用户空间服务，孤儿进程的父进程。
</details>

---

## 交叉引用

- [01-arm64-boot-assembly.md](./01-arm64-boot-assembly.md) — 汇编阶段 head.S
- [chapter-01-kernel-architecture](../../chapter-01-kernel-architecture/) — 内核子系统概览
- [chapter-10-preempt-rt](../../chapter-10-preempt-rt/) — PREEMPT_RT 启动参数
