# vDSO — 虚拟动态共享对象

> **原文:** [vDSO and system calls](https://lwn.net/Articles/627232/) (LWN)
> **内核版本:** 2.6+ (vdso), 6.x (持续优化)
> **对标旧书:** ULK3 Ch10 (系统调用)

---

## 核心观点

vDSO (Virtual Dynamic Shared Object) 是内核映射到用户空间的一小段代码，让部分系统调用**在用户态完成**，避免系统调用开销。

### 为什么需要 vDSO

某些"系统调用"实际上不需要进入内核：

| 调用 | 为什么不需要内核 | 频率 |
|------|----------------|------|
| `gettimeofday()` | 只读内核维护的时间（vvar） | 极高（每秒数千次） |
| `clock_gettime()` | 同上 | 极高 |
| `time()` | 同上 | 高 |
| `getcpu()` | 只读 CPU 拓构信息 | 中 |

### vDSO 工作原理

```
用户空间                          内核空间
┌─────────────┐                  ┌─────────────────┐
│ 应用程序     │                  │ 时钟中断更新      │
│ ↓            │                  │ vvar 页中的时间值 │
│ vDSO 代码页  │ ←── mmap ──→     │ (vvar)           │
│ (vdso.so)    │                  │                  │
│ ↓            │                  │                  │
│ 读取 vvar 页 │ ←── mmap ──→     │ vvar 页 (只读)    │
│ 计算当前时间  │                  │ (gtod_data 等)    │
│ 返回结果      │                  │                  │
└─────────────┘                  └─────────────────┘
    ↑ 无系统调用！
```

1. 内核在进程启动时将 vDSO 代码页和 vvar 数据页映射到用户空间
2. 用户调用 `gettimeofday()` → 动态链接器跳转到 vDSO 中的实现
3. vDSO 代码读取 vvar 页中的时间数据，计算结果，返回用户
4. **整个过程在用户态完成，无系统调用**

### vvar 页内容

```c
// 内核维护的 vvar 数据 (只读映射到用户空间)
struct vdso_data {
    u32 seq;              // 序列号 (奇数=正在更新)
    u32 clock_mode;       // 时钟源类型
    u64 cycle_last;       // 上次时钟读数
    u64 mask;             // 时钟掩码
    u32 mult;             // 乘法因子
    u32 shift;            // 移位因子
    u64 basetime[CS_BASES]; // 基准时间
    // ...
};
```

### x86-64 vs ARM64

| 架构 | vDSO 实现路径 | 时钟源 |
|------|--------------|--------|
| x86-64 | vDSO 中直接读 TSC (rdtsc) | TSC |
| ARM64 | vDSO 中读 CNTVCT_EL0 | Generic Timer |
| 旧 x86 (32) | 通过 int 0x80 回退 | 多种 |

ARM64 的 `CNTVCT_EL0` 读取和 TSC 一样是无开销的硬件计数器，vDSO 在用户态直接读取并计算纳秒时间戳。

---

## 与旧书差异

| ULK3 讲的 | 6.x 现代实现 |
|-----------|-------------|
| `sys_call_table` + `int 0x80` | `syscall` 指令 + vDSO |
| `0x80` 软中断 | 已废弃 |
| `gettimeofday()` 必须进内核 | vDSO 在用户态完成 |
| 无 vDSO 概念 | vDSO 是 6.x 系统调用优化的核心 |

### 关键代码变更

```c
// ULK3 时代 — 系统调用入口
// x86: int 0x80 → system_call() → sys_call_table[nr]

// 6.x — 系统调用入口
// x86-64: syscall 指令 → entry_SYSCALL_64 → sys_call_table[nr]
// 但 gettimeofday / clock_gettime 不走此路径！

// 6.x — vDSO 路径 (无系统调用)
// glibc: gettimeofday() → __vdso_gettimeofday() → vDSO 代码
// vDSO: 读取 vvar 页 → 计算 → 返回
```

### 验证 vDSO

```bash
# 查看 vDSO 映射
$ ldd /bin/ls | grep vdso
linux-vdso.so.1 (0x00007ffe12345678)

# strace 确认 gettimeofday 不产生系统调用
$ strace -e trace=gettimeofday ./myapp
# (无输出 — vDSO 处理了，没有实际系统调用)

# 对比 — 强制不用 vDSO
$ LD_PRELOAD=/lib/libc.so.6 strace -e trace=gettimeofday ./myapp
gettimeofday({tv_sec=...}, NULL) = 0  # 产生了系统调用
```

---

## HFT 关联

| 场景 | vDSO 影响 |
|------|-----------|
| **时间戳获取** | `clock_gettime(CLOCK_MONOTONIC)` 通过 vDSO **零系统调用**，~20ns |
| **每笔交易打时间戳** | vDSO 使得每笔交易记录纳秒时间戳的开销可忽略 |
| **延迟测量** | vDSO 让用户态精确测量代码段耗时 |
| **对比系统调用开销** | vDSO ~20ns vs syscall ~200-500ns，差 10-25 倍 |

> **HFT 实盘：** 交易系统每个关键路径用 `clock_gettime(CLOCK_MONOTONIC)` 打时间戳。vDSO 使其在用户态完成，开销约 20ns（主要是 rdtsc/CNTVCT_EL0 指令 + 数学计算）。如果走系统调用，每次 200-500ns，数千次打戳累计显著。

```c
// HFT 时间戳最佳实践
static inline uint64_t rdtsc_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);  // vDSO，无系统调用
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// 或直接读 TSC (x86) / CNTVCT_EL0 (ARM64) 更快，但需校准
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** vDSO 为什么不需要系统调用就能获取精确时间？

> 内核在时钟中断中更新 vvar 页中的时间数据（基准时间、时钟计数器值、换算因子）。vDSO 代码在用户态读取 vvar 页的只读数据，结合当前 TSC/CNTVCT_EL0 计数器值，计算出当前时间。整个过程是用户态内存读取 + 数学运算，不需要切换到内核态。

**Q2:** vDSO 的 vvar 页如何保证读取的一致性（不被内核更新打断）？

> vvar 页使用序列号 (seq) 机制。内核更新前将 seq 设为奇数（表示正在更新），更新完设为偶数。vDSO 代码读取前检查 seq 为偶数，读取后再检查 seq 未变。如果变了则重试。这类似 seqlock，无锁但保证一致性。

**Q3:** 在 ARM64 树莓派 5 上 `clock_gettime(CLOCK_MONOTONIC)` 大约多快？

> 通过 vDSO 读取 `CNTVCT_EL0` 寄存器 + 数学计算，约 20-40ns。如果走系统调用 (syscall 指令 + 内核处理 + 返回)，约 300-500ns。vDSO 快 10-15 倍。对于每秒调用数千次的 HFT 时间戳，差异显著。

</details>
