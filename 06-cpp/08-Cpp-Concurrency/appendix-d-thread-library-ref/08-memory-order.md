# D.8 memory_order 内存序

> 附录 D · 上一节：[D.7 latch / barrier 屏障](07-latch-barrier.md) · 上一节：[D.7 latch / barrier](07-latch-barrier.md)

## 这节讲什么

`<atomic>` 中的 `memory_order` 枚举控制原子操作的内存序——决定可见性和重排约束。本节是速查参考——六种内存序的语义、配对规则、以及 x86 上的实际编译结果。

---

## 核心规则（代码+表格）

### 六种内存序速查

| 内存序 | 重排约束 | 同步效果 | x86 编译 |
|--------|----------|----------|----------|
| `relaxed` | 无约束 | 无 | `mov` |
| `consume` | 数据依赖（极少用） | 弱 acquire | `mov` |
| `acquire` | 后续读写不重排到前面 | 读端同步 | `mov` |
| `release` | 前面读写不重排到后面 | 写端同步 | `mov` |
| `acq_rel` | acquire + release | 读写都同步 | `lock` / `mfence` |
| `seq_cst` | 全局顺序（最严格） | 全局同步 | `mfence` / `lock` |

### release / acquire 配对（最常用）

```cpp
std::atomic<bool> ready{false};
int data = 0;  // 非原子

// 线程1（生产者）
data = 42;  // 普通写
ready.store(true, std::memory_order_release);
// release：data = 42 不会被重排到 store 之后
// → 消费者看到 ready=true 时，data 一定是 42

// 线程2（消费者）
while (!ready.load(std::memory_order_acquire));
// acquire：后续读不会重排到 load 之前
assert(data == 42);  // 保证成立
```

### `relaxed`：无同步的原子

```cpp
std::atomic<int> counter{0};

// 多线程计数，不关心顺序
void worker() {
    for (int i = 0; i < 1000000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}
// 只保证原子性（不撕裂），不保证可见性顺序
// 适合：纯计数器、统计
```

### `seq_cst`：全局顺序（默认）

```cpp
std::atomic<bool> x{false}, y{false};
int r1, r2;

// 线程1
x.store(true);  // seq_cst（默认）
r1 = y.load();

// 线程2
y.store(true);  // seq_cst（默认）
r2 = x.load();

// seq_cst 保证：不可能 r1==false && r2==false
// （至少一个 store 在另一个 load 之前）
// 如果用 relaxed：可能 r1==false && r2==false（重排）
```

### CAS 的内存序

```cpp
std::atomic<int> a{0};

// CAS 有两个内存序：成功和失败
int expected = 0;
bool success = a.compare_exchange_weak(
    expected, 42,
    std::memory_order_acq_rel,    // 成功：acq_rel（读写都做）
    std::memory_order_relaxed);    // 失败：relaxed（只读）

// 常见配对：
// 成功 acq_rel，失败 relaxed → 通用
// 成功 release，失败 relaxed → 只发布数据
// 成功 seq_cst，失败 seq_cst → 最安全（默认）
```

### 内存序配对规则

| 生产者（写） | 消费者（读） | 效果 |
|-------------|-------------|------|
| `release` | `acquire` | 发布-消费（最常用） |
| `release` | `consume` | 数据依赖（极少用） |
| `seq_cst` | `seq_cst` | 全局顺序 |
| `relaxed` | `relaxed` | 无同步（仅原子） |

### x86 上的实际开销

```cpp
// x86 TSO（Total Store Order）天然满足大部分内存序
// → 很多操作编译为普通 mov，无额外指令

std::atomic<int> a{0};

a.load(std::memory_order_relaxed);    // mov eax, [a]
a.load(std::memory_order_acquire);    // mov eax, [a]（x86 天然 acquire）
a.load(std::memory_order_seq_cst);    // mov eax, [a]（x86 load 天然 seq_cst）

a.store(1, std::memory_order_relaxed);  // mov [a], 1
a.store(1, std::memory_order_release);  // mov [a], 1（x86 天然 release）
a.store(1, std::memory_order_seq_cst);  // mov [a], 1; mfence（需要 fence！）

a.fetch_add(1, std::memory_order_relaxed);  // lock xadd [a], 1
a.fetch_add(1, std::memory_order_acq_rel);  // lock xadd [a], 1（相同）
a.fetch_add(1, std::memory_order_seq_cst);  // lock xadd [a], 1（相同）
```

| 操作 | relaxed | acquire/release | seq_cst |
|------|---------|-----------------|---------|
| load | `mov` | `mov` | `mov` |
| store | `mov` | `mov` | `mov` + `mfence` |
| CAS/fetch | `lock` | `lock` | `lock` |

**结论**：x86 上 acquire/release 和 relaxed 几乎无开销——HFT 应优先用 acquire/release 而非 seq_cst（seq_cst 的 store 多一个 `mfence`）。

---

## 新手要点（和 C 的区别）

- **C 程序员通常不接触内存序**：C 的 `volatile` 不保证内存序——C 程序员要么不管（数据竞争），要么用 `__sync_synchronize`（全屏障，过度）。C++ 的六种 `memory_order` 是精确控制。
- **"为什么需要内存序"是 C 程序员的常见疑问**：CPU 和编译器都会重排指令以优化性能——单线程下无感知，但多线程下重排会破坏同步。内存序告诉编译器/CPU"这里不能重排"。
- **release/acquire 是"刚好够用"的选择**：C 程序员可能觉得"用 seq_cst 最安全"——但 seq_cst 在某些场景有额外开销（x86 的 store 需要 mfence）。release/acquire 是"刚好够用"的精确同步。
- **x86 的 TSO 是 HFT 的优势**：x86 的强内存模型让 acquire/release 几乎零开销——这是 HFT 偏好 x86 的原因之一。ARM 的弱内存模型需要更多屏障指令。

---

## HFT 关联

- **HFT 用 acquire/release 替代 seq_cst**：x86 上 acquire/release 编译为 `mov`，seq_cst 的 store 需要 `mfence`（~20-30 周期）。HFT 热路径用 acquire/release。
- **`relaxed` 用于无序计数器**：HFT 的统计计数器（如总成交量）用 `relaxed`——只保证原子，不关心顺序。
- **CAS 用 acq_rel**：HFT 的无锁结构 CAS 成功用 `acq_rel`（读写都同步），失败用 `relaxed`（只读，无需同步）。
- **ARM 的挑战**：HFT 如果迁移到 ARM（如 Graviton），内存序开销更大——ARM 的弱内存模型需要 `dmb` 屏障指令。x86 代码迁移到 ARM 要重新验证内存序正确性。
- **SPSC 队列的内存序**：HFT 的 SPSC 队列用 acquire/release 配对——生产者 `release` 发布数据，消费者 `acquire` 获取数据。x86 上零额外开销。

---

## 自测题

1. 六种内存序中，`relaxed`/`acquire`/`release`/`seq_cst` 各有什么语义？
2. release/acquire 配对如何实现"发布-消费"同步？
3. 为什么 `relaxed` 只保证原子性不保证可见性？它适合什么场景？
4. x86 上 `seq_cst` 的 store 为什么比 `release` 慢？多了什么指令？
5. HFT 为什么优先用 acquire/release 而非 seq_cst？

---

## 参考与延伸

- 回到：[附录 D](README.md)
- 上一节：[D.7 latch / barrier 屏障](07-latch-barrier.md)
- 上一章：[附录 C.6 完整流程](../appendix-c-atm-example/06-full-flow.md)
