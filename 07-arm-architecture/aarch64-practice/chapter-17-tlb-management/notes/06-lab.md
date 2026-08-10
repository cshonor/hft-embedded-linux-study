# §17.6 实验要点

> **来源：** [Ch17 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

本章以案例分析为主，无独立编号实验。关键案例：Linux 内核 TLB 维护、ASID 切换、BBM 机制。

## 核心要点

### 关键案例

| 案例 | 内容 | 关键点 |
|------|------|--------|
| Linux TLB 维护 | 内核中 TLB 刷新的时机和方式 | vae1 vs alle1 选择 |
| ASID 切换 | 进程切换时 ASID 设置 | TTBR0 高位写 ASID |
| BBM 机制 | 修改页表的安全协议 | break → flush → DSB → make |

### 推荐实践

1. 阅读 Linux 内核 `arch/arm64/mm/tlb.S` 中的 `__tlb_switch_to_guest` 等函数
2. 用 QEMU `-d int` 选项观察 TLB 操作
3. 在裸金属代码中手动修改页表 + TLB 刷新，验证 BBM

## HFT 关联

虽然本章无独立实验，但 TLB 管理知识在 HFT 中有实际应用：1) 用大页减少 TLB miss；2) 避免 `mprotect`/`munmap` 触发 TLB 刷新；3) 线程绑定 CPU 避免进程切换 TLB flush。建议在 QEMU 上实验手动 TLB 操作（`tlbi vae1`），理解 TLB 刷新对后续访存的影响——刷新后第一次访问会 TLB miss，延迟增加。

## 自测题

1. **Linux 的 `arch/arm64/mm/tlb.S` 中 `flush_tlb_mm` 函数用什么 TLBI 指令？**

<details>
<summary>答案</summary>

用 `tlbi aside1is, x0`（Inner Shareable，按 ASID 刷）。因为 `flush_tlb_mm` 刷整个进程的 TLB（不是单个 VA），用 ASID 精确刷该进程。`is`（Inner Shareable）确保所有核上的该 ASID 条目都被刷新。
</details>

2. **如何用 QEMU 观察 TLB 操作？**

<details>
<summary>答案</summary>

用 `-d int` 选项打印中断/异常信息，包括 TLB 相关操作：
```bash
qemu-system-aarch64 -d int -D qemu_log.txt ...
```
日志中可以看到 TLB miss、页表 walk 等信息。也可以用 `-d mmu` 选项专门查看 MMU/TLB 操作。
</details>

3. **在裸金属代码中如何验证 BBM 的必要性？**

<details>
<summary>答案</summary>

实验：两个核，核 A 修改页表映射（VA→PA1 改为 VA→PA2），核 B 持续读该 VA。
- 不遵循 BBM（直接改）：核 B 可能在 TLB 刷新前读到 PA1 的旧数据
- 遵循 BBM（break→flush→DSB→make）：核 B 在 break 后 TLB miss，等到 make 后才读到 PA2 的新数据

对比两种情况的核 B 读到的值，验证 BBM 的必要性。
</details>

## 参考与延伸

- [§17.3 TLB 刷新指令](03-tlb-flush.md) — 实验中使用的 TLBI 指令
- [§17.4 BBM](04-bbm.md) — BBM 协议详解
- [§17.5 内核 TLB 维护场景](05-tlb-scenarios.md) — Linux 内核的 TLB 操作
