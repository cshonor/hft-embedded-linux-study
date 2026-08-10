# §20.3 ARMv8.1 LSE（Large System Extensions）

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8.1 引入单指令原子操作（LSE），如 LDADD（原子加）、CAS（原子比较交换）、SWP（原子交换），不再需要 LDXR/STXR 循环。Pi5 Cortex-A76 支持 LSE。

## 核心要点

### LSE 指令集

| 指令 | 行为 |
|------|------|
| `LDADD Ws, Wd, [Xn]` | 原子加法：`Wd = [Xn]; [Xn] += Ws` |
| `CAS Ws, Wt, [Xn]` | 原子 CAS：如果 `[Xn]==Ws` 则 `[Xn]=Wt` |
| `SWP Ws, Wd, [Xn]` | 原子交换：`Wd = [Xn]; [Xn] = Ws` |
| `LDSET Ws, Wd, [Xn]` | 原子置位：`Wd = [Xn]; [Xn] \|= Ws` |
| `LDCLR Ws, Wd, [Xn]` | 原子清位：`Wd = [Xn]; [Xn] &= ~Ws` |

### LSE vs LDXR/STXR

| 特性 | LDXR/STXR 循环 | LSE 单指令 |
|------|---------------|-----------|
| 指令数 | 4+（LDXR+修改+STXR+CBNZ） | 1 |
| 重试 | 高竞争时反复失败 | 无重试 |
| 延迟（低竞争） | ~10-15ns | ~5ns |
| 延迟（高竞争） | ~50-100ns（livelock） | ~5-10ns |
| 可用性 | 所有 ARMv8 | ARMv8.1+ |

> LSE 比 LDXR/STXR 循环**更快**（无重试开销），在多核高竞争场景尤其明显。
> Linux 通过 `__LSE_ATOMIC` 宏在编译期选择。Pi5 Cortex-A76 支持 LSE。

### 编译期选择

```c
// Linux 内核通过配置选择
#ifdef CONFIG_ARM64_LSE_ATOMICS
    // 用 LSE 指令
    ldadd w0, w1, [x2]
#else
    // 用 LDXR/STXR 循环
1:  ldxr w1, [x2]
    add  w1, w1, w0
    stxr w3, w1, [x2]
    cbnz w3, 1b
#endif
```

## HFT 关联

LSE 对 HFT 高竞争场景有显著优势——LDXR/STXR 在高竞争时可能 livelock（反复失败重试），延迟不可预测。LSE 单指令原子操作延迟固定（~5ns），不受竞争程度影响。Pi5 的 Cortex-A76 支持 LSE，HFT 代码编译时应启用 `-march=armv8.1-a` 或 `-mcpu=cortex-a76` 让编译器使用 LSE 指令。在 DPDK 中，LSE 可以显著提升多核网络包计数器的性能。Linux 通过 `CONFIG_ARM64_LSE_ATOMICS` 配置。

## 自测题

1. **LSE 相比 LDXR/STXR 循环有什么优势？为什么在高竞争时差距更大？**

<details>
<summary>答案</summary>

LSE 优势：
1. **单指令完成**（如 LDADD），无循环重试
2. **延迟固定**（~5ns），不受竞争影响

高竞争时差距更大：LDXR/STXR 循环在高竞争时多个核同时 LDXR 同一地址，只有一个 STXR 成功，其他失败重试——竞争越激烈重试越多，延迟暴增（livelock）。LSE 硬件保证原子完成，无重试，延迟始终 ~5-10ns。
</details>

2. **`LDADD Ws, Wd, [Xn]` 的行为是什么？和 LDXR/STXR 循环等价吗？**

<details>
<summary>答案</summary>

`LDADD` 行为：`Wd = [Xn]`（返回旧值到 Wd），`[Xn] += Ws`（原子加法）。等价于：
```asm
1:  ldxr Wd, [Xn]
    add  Wt, Wd, Ws
    stxr Ws2, Wt, [Xn]
    cbnz Ws2, 1b
```
但 LDADD 是单指令，不需要循环重试。功能等价但性能更好。
</details>

3. **Pi5 Cortex-A76 支持 LSE 吗？如何在编译时启用？**

<details>
<summary>答案</summary>

**支持**。Cortex-A76 实现了 ARMv8.2-A，包含 LSE（ARMv8.1 引入）。编译时启用：
- GCC: `-march=armv8.1-a` 或 `-mcpu=cortex-a76`（自动启用 LSE）
- 或用 `-march=armv8-a+lse` 明确指定
- Linux 内核：`CONFIG_ARM64_LSE_ATOMICS=y`

不启用 LSE 的话编译器仍用 LDXR/STXR 循环，性能在高竞争时下降。
</details>

## 参考与延伸

- [§20.2 原子操作实现模式](02-atomic-patterns.md) — LDXR/STXR 循环代码
- [§20.5 Linux 原子操作 API](05-linux-atomic-api.md) — 内核如何选择 LSE/LDXR
- [Ch19 §19.6 HFT 中的屏障使用](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — HFT 无锁队列
