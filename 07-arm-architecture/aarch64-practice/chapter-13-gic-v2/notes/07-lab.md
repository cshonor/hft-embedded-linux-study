# §13.7 实验要点

> **来源：** [Ch13 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章 2 个实验：通用定时器中断（GICv2 完整流程）和树莓派系统定时器。从 QEMU 验证到 Pi4B 实机，Pi5 需改 GICv3 接口。包含完整代码、编译命令和调试方法。

## 核心要点

| 实验 | 内容 | 平台 | Pi5 适配 | 关键技能 |
|------|------|------|----------|----------|
| 13-1 | 通用定时器中断（GICv2 流程） | QEMU | QEMU 可配 GICv2 | GIC 初始化 + ISR |
| 13-2 | 树莓派系统定时器 | Pi4B | Pi5=GICv3，需改接口 | 实机调试 + GICv3 |

### 实验 13-1：通用定时器中断完整实现

#### 完整代码框架

```c
// gic.h — GICv2 寄存器定义
#define GICD_BASE 0x08000000
#define GICC_BASE 0x08010000

// gic.c — GIC 初始化 + 中断处理
#include "gic.h"

void gic_init(void) {
    // 1. 使能 Distributor
    *(volatile uint32_t *)(GICD_BASE + 0x000) = 1;

    // 2. 使能 CPU Interface
    *(volatile uint32_t *)(GICC_BASE + 0x000) = 1;

    // 3. 设优先级掩码（必须！默认 0 屏蔽所有）
    *(volatile uint32_t *)(GICC_BASE + 0x004) = 0xFF;

    // 4. 使能定时器中断 (IRQ 30)
    *(volatile uint32_t *)(GICD_BASE + 0x100) |= (1 << 30);

    // 5. 设目标 CPU0 (ITARGETSR)
    //    30/4=7 (第7组), 30%4=2 (第2个8位槽)
    *(volatile uint32_t *)(GICD_BASE + 0x800 + 7*4) |= (1 << 16);

    // 6. 设优先级
    *(volatile uint8_t *)(GICD_BASE + 0x400 + 30) = 0xA0;
}

void gic_handle_irq(void) {
    uint32_t irq = *(volatile uint32_t *)(GICC_BASE + 0x00C); // IAR

    if (irq == 1023) return;  // spurious

    switch (irq) {
        case 30:  timer_isr(); break;
        default:  break;
    }

    *(volatile uint32_t *)(GICC_BASE + 0x010) = irq;  // EOIR
}
```

#### 定时器 ISR

```c
// timer.c — 通用定时器中断
#define CNTP_CTL_EL0    "cntp_ctl_el0"
#define CNTP_TVAL_EL0   "cntp_tval_el0"
#define CNTPCT_EL0      "cntpct_el0"
#define CNTFRQ_EL0      "cntfrq_el0"

static uint64_t timer_frequency;

void timer_init(uint32_t timeout_us) {
    uint64_t freq;
    asm volatile("mrs %0, " CNTFRQ_EL0 : "=r"(freq));
    timer_frequency = freq;

    // 设 TVAL = timeout 微秒对应的 cycle 数
    uint64_t tval = freq / 1000000 * timeout_us;
    asm volatile("msr " CNTP_TVAL_EL0 ", %0" :: "r"(tval));

    // 使能定时器 + 使能中断
    // CTL: bit0=使能, bit1=IMASK(0=不屏蔽)
    asm volatile("msr " CNTP_CTL_EL0 ", #1");
}

void timer_isr(void) {
    // 1. 重设 TVAL（清除中断源！电平触发必须先清除）
    uint64_t tval = timer_frequency / 1000000 * 1000; // 1ms
    asm volatile("msr " CNTP_TVAL_EL0 ", %0" :: "r"(tval));

    // 2. 处理定时器逻辑
    uart_puts("Timer tick!\n");
}
```

#### 编译和运行

```bash
# 编译
aarch64-linux-gnu-gcc -c -o start.o start.S
aarch64-linux-gnu-gcc -c -o gic.o gic.c
aarch64-linux-gnu-gcc -c -o timer.o timer.c
aarch64-linux-gnu-gcc -c -o main.o main.c
aarch64-linux-gnu-ld -T linker.ld -o kernel.elf start.o gic.o timer.o main.o
aarch64-linux-gnu-objcopy -O binary kernel.elf kernel8.img

# QEMU 运行（GICv2 模式）
qemu-system-aarch64 -machine virt,gic-version=2 \
    -cpu cortex-a72 -m 128M \
    -kernel kernel8.img -nographic
```

### 实验 13-2：Pi5 适配清单

| 原书 (Pi4B/GICv2) | Pi5 (GICv3) | 改动说明 |
|-------------------|-------------|----------|
| `GICC->IAR` | `mrs x0, ICC_IAR1_EL1` | 系统寄存器替代 MMIO |
| `GICC->EOIR = irq` | `msr ICC_EOIR1_EL1, x0` | 系统寄存器替代 MMIO |
| GICD+GICC 两步初始化 | GICD+GICR+ICC 三步初始化 | 多 Redistributor |
| ITARGETSR 设目标 CPU | GICR 自动路由 | PPI 不需要设目标 |
| GIC 基址 0xFF841000 | BCM2712 新地址 | 查数据手册 |
| GICD_ICFGR 设触发类型 | GICR_ICFGR (PPI) / GICD_ICFGR (SPI) | PPI 在 GICR 设 |

#### 中断延迟测量方法

```c
// 在 timer_init 前记录时间
uint64_t t1, t2;
asm volatile("mrs %0, cntpct_el0" : "=r"(t1));

// 在 ISR 入口记录时间
void timer_isr(void) {
    asm volatile("mrs %0, cntpct_el0" : "=r"(t2));

    // 中断延迟 = (t2 - t1 - tval_cycles) / freq * 1e9 (纳秒)
    uint64_t latency_cycles = t2 - t1;
    uint64_t latency_ns = latency_cycles * 1000000000ULL / timer_frequency;

    uart_hex(latency_ns);
    uart_puts(" ns\n");

    // 重设 TVAL
    asm volatile("msr cntp_tval_el0, %0" :: "r"(timer_frequency / 1000));
}
```

### 常见问题排查表

| 症状 | 可能原因 | 检查方法 |
|------|----------|----------|
| 中断完全不触发 | PMR=0 / ITARGETSR=0 / ISENABLER 未设 | 打印 GICD/GICC 寄存器值 |
| 中断只触发一次 | EOIR 没写 / EOIR 写错值 | 检查 gic_handle_irq 的 EOIR 路径 |
| 中断疯狂重入 | TVAL 没重设（电平触发） | 检查 ISR 是否先清中断源 |
| 1023 spurious | 多核竞争 / 中断已清除 | 检查是否多核同时响应 |
| Pi5 无响应 | GICv2 代码跑在 GICv3 上 | 确认 GIC 版本 |

## HFT 关联

实验 13-1 是 HFT 定时器中断的完整实现——从 GIC 初始化到 ISR 处理，每一步都需要正确。建议在 QEMU 上完成后，用 `CNTPCT_EL0` 测量从中断触发到 ISR 执行的时间（中断延迟基准）。实验 13-2 的 Pi5 适配是实际 HFT 部署的必经之路——理解 GICv2→GICv3 的差异才能在 Pi5 上正确使用中断。

## 自测题

1. **实验 13-1 中，GIC 初始化后中断不工作，最可能的原因是什么？**

<details>
<summary>答案</summary>

最可能原因：**忘记设 GICC_PMR=0xFF**（默认 0 屏蔽所有中断）或**忘记设 ITARGETSR**（默认 0 = 无目标 CPU）。其他可能：GICD_CTLR 或 GICC_CTLR 没使能、ISENABLER 没设对应位。
</details>

2. **实验 13-2 在 Pi5 上需要修改哪些代码？**

<details>
<summary>答案</summary>

1. GIC 基址改为 BCM2712 的地址（查数据手册）
2. 中断确认从 `GICC->IAR` 改为 `mrs x0, ICC_IAR1_EL1`
3. 结束中断从 `GICC->EOIR = irq` 改为 `msr ICC_EOIR1_EL1, x0`
4. 初始化流程增加 Redistributor（GICR）配置
5. ITARGETSR 可能不需要（GICv3 的 GICR 自动路由 PPI）
6. 先设 ICC_SRE_EL1.SRE=1 启用系统寄存器模式
</details>

3. **如何测量中断延迟（从定时器触发到 ISR 开始执行）？**

<details>
<summary>答案</summary>

方法：在设置 CNTP_TVAL 前读 `CNTPCT_EL0` 记录时间 T1。在 ISR 入口（保存完现场后）读 `CNTPCT_EL0` 记录 T2。中断延迟 = T2 - T1（转换为纳秒：`(T2-T1) * 1e9 / CNTFRQ`）。注意：T1 到实际触发之间有 TVAL 倒计时间，需要精确计算。

实际延迟 = (T2 - T1) - TVAL_cycles，再转换为纳秒。
</details>

4. **ISR 中为什么要先重设 TVAL 再处理逻辑？顺序反过来会怎样？**

<details>
<summary>答案</summary>

先重设 TVAL 是为了**尽快清除中断源**（定时器电平触发，信号保持高直到 TVAL 重设）。如果先处理逻辑再重设 TVAL：处理逻辑耗时长 → 中断信号保持高更久 → 写 EOIR 后 GIC 立即再发中断 → ISR 重入。

更严重的情况：如果处理逻辑中有 while 循环等待，中断信号一直高，系统可能死在无限中断中。先清 TVAL → 中断信号拉低 → 安全处理 → 写 EOIR → 不会重入。
</details>

## 参考与延伸

- [§13.3 GIC 初始化流程](03-gic-init.md) — 实验 13-1 的初始化代码
- [§13.4 中断处理流程](04-irq-flow.md) — 实验 13-1 的 ISR 代码
- [§13.6 GICv2 vs GICv3](06-gicv2-vs-gicv3.md) — 实验 13-2 的 Pi5 适配
- [Ch12 §12.3 通用定时器](../../chapter-12-interrupt-handling/notes/03-timer-interrupt.md) — 定时器寄存器详解
