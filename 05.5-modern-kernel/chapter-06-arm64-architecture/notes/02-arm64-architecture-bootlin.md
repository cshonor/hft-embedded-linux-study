# Bootlin: ARM64 架构基础

> **来源:** [Bootlin ARM64 Training](https://bootlin.com/docs/arm/)
> **主题:** AArch64 架构与内核适配
> **对标旧书:** ULK3 (x86 only) / 03-arm-architecture 模块

---

## 讲义要点

### ARM64 (AArch64) vs x86-64

| 特性 | x86-64 | ARM64 (AArch64) |
|------|--------|-----------------|
| **指令集** | CISC, 变长指令 | RISC, 定长 4 字节 |
| **寄存器** | 16 GPR (rax-r15) | 31 GPR (x0-x30) |
| **调用约定** | rdi,rsi,rdx,rcx,r8,r9 | x0-x7 |
| **栈指针** | rsp | sp (x31) |
| **链接寄存器** | 无（压栈保存返回地址） | x30 (lr) |
| **程序计数器** | rip | pc |
| **页表** | 4 级 (或 5 级) | 3-4 级 (可配置) |
| **内存模型** | TSO (Total Store Order) | Weak (弱排序) |
| **原子指令** | lock 前缀 | LDXR/STXR (LL/SC) |

### ARM64 异常等级

```
EL0 — 用户空间 (User)
EL1 — 内核空间 (Kernel) ← Linux 内核运行在此
EL2 — Hypervisor (KVM)
EL3 — Secure Monitor (TrustZone)
```

### ARM64 内存模型 (弱排序)

ARM64 是**弱排序内存模型**——CPU 可以乱序执行和重排内存访问，需要内存屏障指令保证顺序：

| 屏障指令 | 作用 | x86 对应 |
|---------|------|---------|
| `DMB ISH` | 数据内存屏障 (inner shareable) | `mfence` |
| `DSB ISH` | 数据同步屏障 (更严格) | `mfence` (更强) |
| `ISB` | 指令同步屏障 (刷新流水线) | 无直接对应 |
| `LDAR` | 获取读 (load-acquire) | 无 (x86 天然 acquire) |
| `STLR` | 释放写 (store-release) | 无 |

> **HFT 关键:** ARM64 弱排序意味着无屏障的共享变量读取可能看到旧值。内核的 `READ_ONCE()` / `WRITE_ONCE()` 和 `smp_rmb()` / `smp_wmb()` 在 ARM64 上生成实际屏障指令，在 x86 上可能是空操作。

### ARM64 原子操作 (LL/SC)

```asm
// ARM64 原子加法 (LDXR/STXR 实现)
atomic_add:
1:  ldxr    w1, [x0]        // 独占加载
    add     w1, w1, #1       // 加 1
    stxr    w2, w1, [x0]     // 独占存储
    cbnz    w2, 1b           // 如果失败则重试
    ret

// 6.x 内核也支持 LSE (Large System Extension) 原子指令
// STADD w1, [x0]  — 单条原子加法指令（如果硬件支持）
```

### ARM64 页表 (4K/64K 页)

| 配置 | 页大小 | 页表级数 | 虚拟地址空间 |
|------|--------|---------|-------------|
| 4K 页 | 4KB | 4 级 (48-bit VA) | 256TB |
| 4K 页 + 52-bit VA | 4KB | 5 级 | 4PB |
| 64K 页 | 64KB | 3 级 (42-bit VA) | 4TB |
| 64K 页 + 52-bit VA | 64KB | 3 级 | 4PB |

树莓派 5 (Cortex-A76) 默认使用 4KB 页 + 4 级页表 (39-bit 或 48-bit VA)。

---

## 动手实验

```bash
# 1. 查看 ARM64 CPU 信息
cat /proc/cpuinfo
# processor : 0-3
# model name : Cortex-A76
# BogoMIPS : 108.00
# Features : fp asimd evtstrm aes pmull sha1 sha2 crc32 ...

# 2. 查看页大小和内存布局
getconf PAGE_SIZE       # 4096
cat /proc/self/maps     # 查看进程地址空间布局

# 3. 查看内核支持的 ARM64 特性
dmesg | grep -i "features\|alternatives"
# [    0.000000] CPU features: detected: Spectre-v2
# [    0.000000] CPU features: detected: ARM64_HW_AF_DBM

# 4. 反汇编内核函数 (ARM64)
# 需要 vmlinux (未压缩内核)
aarch64-linux-gnu-objdump -d vmlinux | grep -A 20 "<schedule>"

# 5. 检查 LSE 原子指令支持
cat /proc/cpuinfo | grep Features
# 有 "atomics" 表示支持 LSE

# 6. 内存屏障开销测试
# ARM64 的 DMB 比 x86 的 MFENCE 开销大
# 用 perf 测量
perf stat -e armv8_pmuv3/bus_access ./membench
```

---

## 与旧书差异

| ULK3 (x86) | Bootlin ARM64 |
|------------|---------------|
| x86-64 only | ARM64 对口树莓派 5 |
| TSO 内存模型 | 弱排序，需显式屏障 |
| lock 前缀原子 | LL/SC (LDXR/STXR) |
| 4 级页表固定 | 可配置页大小和级数 |
| `cr3` 页表基址 | `TTBR0_EL1` / `TTBR1_EL1` (用户/内核分离) |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** ARM64 弱排序内存模型对内核开发有什么影响？

> x86 (TSO) 保证 store-store 和 load-load 有序，内核代码中的许多共享变量访问不需要显式屏障。ARM64 (弱排序) 下，所有无屏障的共享变量访问都可能被重排。内核的 `READ_ONCE()` / `WRITE_ONCE()` / `smp_mb()` / `smp_rmb()` 在 ARM64 上生成实际屏障指令，在 x86 上可能是编译器屏障 (空操作)。在 ARM64 上忘记加屏障会导致难以复现的并发 bug。

**Q2:** ARM64 的 `LDAR` / `STLR` (acquire/release) 与 `DMB` 屏障有什么区别？

> `DMB` 是显式屏障指令，阻止屏障前后的内存操作重排。`LDAR` / `STLR` 是带 acquire/release 语义的加载/存储指令，只影响该指令与其他内存操作的顺序。acquire/release 通常比 DMB 开销小，且语义更精确。6.x 内核在 ARM64 上优先使用 `LDAR` / `STLR`。

**Q3:** 树莓派 5 (Cortex-A76) 用 4KB 页还是 64KB 页？有什么影响？

> 默认用 4KB 页 + 4 级页表。4KB 页的 TLB 覆盖范围较小，对大内存应用 TLB miss 率高。64KB 页 TLB 覆盖范围大，但内部碎片更多。HFT 场景下页表遍历 (page table walk) 延迟显著，64KB 页可减少 TLB miss，但需要重新编译内核 (`CONFIG_ARM64_64K_PAGES=y`)。

</details>
