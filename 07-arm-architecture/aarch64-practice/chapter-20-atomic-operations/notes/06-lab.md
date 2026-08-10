# §20.6 实验要点

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章以案例分析为主。关键案例：独占监视器工作原理、CAS 实现、WFE 自旋锁。

## 核心要点

### 关键案例

| 案例 | 内容 | 关键知识点 |
|------|------|-----------|
| 独占监视器 | LDXR/STXR 的监视和清除机制 | 缓存行粒度监视 |
| CAS 实现 | LDXR+CMP+STXR 循环 | 原子比较交换 |
| WFE 自旋锁 | 低功耗自旋锁 | WFE/SEV 配合 |

### 推荐实践

1. 在 QEMU 双核上测试 LDXR/STXR 的竞争行为
2. 对比普通自旋锁和 WFE 自旋锁的功耗/性能
3. 测试 LSE（如果 QEMU 支持 ARMv8.1）vs LDXR/STXR 的性能差异
4. 用 `perf stat` 测量原子操作的周期数

## HFT 关联

这些案例直接对应 HFT 无锁编程的实际应用。独占监视器的缓存行粒度知识帮助避免伪共享导致的原子操作性能下降。CAS 实现是 HFT 订单簿并发更新的基础。WFE 自旋锁在 HFT 中可以减少等待时的功耗和总线干扰。建议在 Pi5（支持 LSE）上对比 LSE 和 LDXR/STXR 的性能差异——在高竞争场景下 LSE 的优势最明显。

## 自测题

1. **如何在 QEMU 上测试独占监视器的竞争行为？**

<details>
<summary>答案</summary>

启动两个核，都对同一地址执行 LDXR/STXR 循环（如原子计数器自增）。用 `CNTPCT_EL0` 测量每次成功的 STXR 需要多少 cycle。在高竞争下，STXR 失败率升高，每次成功需要更多重试，cycle 数增加。对比低竞争（每核独立地址）和高竞争（同一地址）的性能差异。
</details>

2. **如何对比普通自旋锁和 WFE 自旋锁的性能？**

<details>
<summary>答案</summary>

编写两个版本的自旋锁（普通 LDXR/STXR 循环 vs WFE 版本），在多核上并发获取/释放锁。测量：
1. 获取锁的平均延迟（cycle 数）
2. 总线带宽消耗（普通自旋锁的 LDXR 消耗更多带宽）
3. 功耗（WFE 版本更低，需硬件功耗计）

预期结果：WFE 版本功耗更低、总线带宽更少，但获取延迟可能略高（WFE 唤醒延迟）。
</details>

3. **如何验证 LSE 是否被启用？**

<details>
<summary>答案</summary>

1. 编译时用 `-march=armv8.1-a` 或 `-mcpu=cortex-a76`
2. 反汇编目标代码，查找 `LDADD`/`CAS`/`SWP` 等 LSE 指令（而非 LDXR/STXR 循环）
3. 或运行时读 `ID_AA64ISAR0_EL1` 寄存器的 atomic 字段（bit[23:20]），非零表示支持 LSE
4. Linux 中查 `/proc/cpuinfo` 的 Features 是否包含 `atomics`
</details>

## 参考与延伸

- [§20.1 独占监视器](01-exclusive-monitor.md) — 实验基础
- [§20.3 ARMv8.1 LSE](03-lse.md) — LSE 性能对比
- [§20.4 WFE/SEV](04-wfe-sev.md) — WFE 自旋锁实现
