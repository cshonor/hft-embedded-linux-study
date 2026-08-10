# §20.3 ARMv8.1 LSE（Large System Extensions）

> **来源：** [Ch20 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

ARMv8.1 引入单指令原子操作（LSE），如 LDADD（原子加）、CAS（原子比较交换）、SWP（原子交换），不再需要 LDXR/STXR 循环。本节分析 LSE 指令集、与 LDXR/STXR 的性能对比，以及编译期选择机制。

## 核心要点

### LSE 指令集

| 指令 | 行为 | 等价 LDXR/STXR |
|------|------|---------------|
| `LDADD Ws, Wd, [Xn]` | 原子加法：`Wd = [Xn]; [Xn] += Ws` | LDXR + ADD + STXR |
| `CAS Ws, Wt, [Xn]` | 原子 CAS：如果 `[Xn]==Ws` 则 `[Xn]=Wt` | LDXR + CMP + STXR |
| `SWP Ws, Wd, [Xn]` | 原子交换：`Wd = [Xn]; [Xn] = Ws` | LDXR + STXR |
| `LDSET Ws, Wd, [Xn]` | 原子置位：`Wd = [Xn]; [Xn] \|= Ws` | LDXR + ORR + STXR |
| `LDCLR Ws, Wd, [Xn]` | 原子清位：`Wd = [Xn]; [Xn] &= ~Ws` | LDXR + AND + STXR |
| `LDCLR` | 原子清位 | LDXR + BIC + STXR |
| `LDOR` | 原子或 | LDXR + ORR + STXR |
| `LDCHG` | 原子取反 | LDXR + EOR + STXR |
| `CAS` | 原子比较交换 | LDXR + CMP + STXR |

### LSE 指令的 acquire/release 变体

| 后缀 | 语义 | 说明 |
|------|------|------|
| 无 | relaxed | 无顺序保证 |
| `a` | acquire | 如 `LDADDA` = Load-Acquire 版本 |
| `l` | release | 如 `LDADDL` = Store-Release 版本 |
| `al` | acq_rel | 如 `LDADDAL` = 同时 acquire+release |

```asm
// relaxed 版本（无屏障）
ldadd w0, w1, [x2]     // 原子加，无顺序保证

// acquire 版本
ldadda w0, w1, [x2]    // 后续访存不重排到此之前

// release 版本
ldaddl w0, w1, [x2]    // 前面访存不重排到此之后

// acq_rel 版本
ldaddal w0, w1, [x2]   // 同时 acquire+release
```

### LSE vs LDXR/STXR

| 特性 | LDXR/STXR 循环 | LSE 单指令 |
|------|---------------|-----------|
| 指令数 | 4+（LDXR+修改+STXR+CBNZ） | 1 |
| 重试 | 高竞争时反复失败 | 无重试 |
| 延迟（低竞争） | ~10-15ns | ~5ns |
| 延迟（高竞争） | ~50-100ns（livelock） | ~5-10ns |
| 可用性 | 所有 ARMv8 | ARMv8.1+ |
| 屏障支持 | 需额外 DMB | 自带 a/l/al 变体 |

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

// GCC 编译选择
// -march=armv8.1-a → 用 LSE
// -march=armv8-a+lse → 明确指定 LSE
// -march=armv8-a → 用 LDXR/STXR
// -mcpu=cortex-a76 → 自动启用 LSE
```

### 运行时检测

```c
// 检查 CPU 是否支持 LSE
bool has_lse(void) {
    uint64_t isar0;
    asm volatile("mrs %0, ID_AA64ISAR0_EL1" : "=r"(isar0));
    return ((isar0 >> 20) & 0xf) != 0;  // atomic 字段
}

// Linux 中检查
// /proc/cpuinfo → Features: ... atomics ...
```

## HFT 关联

LSE 对 HFT 高竞争场景有显著优势——LDXR/STXR 在高竞争时可能 livelock（反复失败重试），延迟不可预测。LSE 单指令原子操作延迟固定（~5ns），不受竞争程度影响。

### HFT LSE 性能对比

| 场景 | LDXR/STXR | LSE | 加速 |
|------|-----------|-----|------|
| 低竞争 CAS | ~10ns | ~5ns | 2x |
| 中竞争 CAS | ~30ns | ~5ns | 6x |
| 高竞争 CAS | ~50-100ns | ~5-10ns | 5-10x |
| 原子加法（低竞争） | ~15ns | ~5ns | 3x |
| 原子加法（高竞争） | ~50ns | ~5ns | 10x |

Pi5 的 Cortex-A76 支持 LSE，HFT 代码编译时应启用 `-march=armv8.1-a` 或 `-mcpu=cortex-a76` 让编译器使用 LSE 指令。在 DPDK 中，LSE 可以显著提升多核网络包计数器的性能。Linux 通过 `CONFIG_ARM64_LSE_ATOMICS` 配置。

### HFT 编译选项

```bash
# GCC/Clang 编译选项
gcc -O2 -march=armv8.1-a -o hft_app hft_app.c
# 或明确指定 LSE
gcc -O2 -march=armv8-a+lse -o hft_app hft_app.c
# 或指定 CPU 型号
gcc -O2 -mcpu=cortex-a76 -o hft_app hft_app.c

# 验证是否使用了 LSE
objdump -d hft_app | grep -E "ldadd|cas|swp|ldset"
# 应看到 LSE 指令而非 ldxr/stxr 循环
```

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

4. **`LDADDAL` 后缀 `al` 表示什么语义？等价于 C++ 的哪个内存序？**

<details>
<summary>答案</summary>

`LDADDAL` 的 `al` 表示 **acq_rel**（acquire + release）：
- acquire：后续访存不能重排到此操作前
- release：前面访存不能重排到此操作后

等价于 C++ `atomic.fetch_add(val, std::memory_order_acq_rel)`。其他变体：
- `LDADD`（无后缀）= relaxed
- `LDADDA`（a）= acquire
- `LDADDL`（l）= release
</details>

## 参考与延伸

- [§20.2 原子操作实现模式](02-atomic-patterns.md) — LDXR/STXR 循环代码
- [§20.5 Linux 原子操作 API](05-linux-atomic-api.md) — 内核如何选择 LSE/LDXR
- [Ch19 §19.6 HFT 中的屏障使用](../../chapter-19-barrier-usage/notes/section-0-本章完整概述.md) — HFT 无锁队列
