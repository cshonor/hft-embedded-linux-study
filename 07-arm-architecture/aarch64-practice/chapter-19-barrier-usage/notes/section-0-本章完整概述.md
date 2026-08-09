# Ch19 完整总结 · 合理使用内存屏障

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

Ch18 讲了屏障指令是什么，本章分析 Linux 内核中 **4 次真实屏障使用**，理解何时该加、加什么类型、加在哪。

---

## 19.1 案例一：自旋锁获取/释放 ⭐

```c
// Linux spinlock 简化版
void spin_lock(spinlock_t *lock) {
    while (atomic_cmpxchg(&lock->val, 0, 1) != 0)
        ;  // 自旋等待
    smp_mb();  // ← 获取锁后加屏障
}

void spin_unlock(spinlock_t *lock) {
    smp_mb();  // ← 释放锁前加屏障
    atomic_set(&lock->val, 0);
}
```

**为什么？**
- `lock` 后：保证临界区内的访存不被重排到锁获取之前（其他核可能还在临界区）
- `unlock` 前：保证临界区内的写在锁释放前对其他核可见

> **现代 ARM 内核**：用 LDAR/STLR 替代显式 DMB，更高效。

---

## 19.2 案例二：消息传递（邮箱）

```c
// 生产者
msg->data = payload;
msg->ready = true;
smp_wmb();  // ← 保证 data 写在 ready 之前

// 消费者
while (!msg->ready)
    smp_rmb();  // ← 保证读 ready 后再读 data
use(msg->data);
```

**为什么只需要 wmb/rmb 而不是 mb？**
- 生产者只需要 Store-Store 屏障（`smp_wmb` = `dmb ishst`）
- 消费者只需要 Load-Load 屏障（`smp_rmb` = `dmb ishld`）
- 不需要约束 Load-Store 或 Store-Load → 用更弱的屏障省性能

---

## 19.3 案例三：DMA 操作

```c
// CPU 准备数据给 DMA 读取
prepare_tx_buffer(buf, len);
// 必须用 DSB（不是 DMB！）
mb();  // = dsb sy，确保数据完全写入内存（对 DMA 可见）
start_dma(DMA_TX, buf, len);

// DMA 写完后 CPU 读取
wait_dma_complete();
rmb();  // = dsb ld，确保 DMA 写入对 CPU 可见
process_rx_buffer(buf, len);
```

**为什么用 DSB 而不是 DMB？**
- DMB 只保证访存顺序，CPU 可能继续执行 → `start_dma()` 可能先执行
- DSB 完全停住 CPU → 确保 `prepare_tx_buffer` 写入完成后才执行 `start_dma`

> DMA 场景必须用 DSB，不能用 DMB。这是 Ch18 的知识点在实战中的应用。

---

## 19.4 案例四：TLB 维护

```c
// 修改页表项后
set_pte(ptep, new_pte);
// 必须：先失效 TLB
tlbi vae1is, vaddr;   // Inner Shareable，刷指定 VA
dsb ish;               // 等 TLB 刷新完成
isb;                    // 重新取指
```

**为什么用 DSB ISH + ISB？**
- `dsb ish`：等 TLB 刷新在所有 Inner Shareable CPU 上完成
- `isb`：本核流水线中可能有旧指令使用旧 TLB → 必须冲刷

---

## 19.5 屏障选择决策树

```
需要屏障？
├── 编译器重排？ → barrier()（无硬件开销）
├── CPU 间同步？
│   ├── 只约束 Store？ → smp_wmb()（dmb ishst）
│   ├── 只约束 Load？ → smp_rmb()（dmb ishld）
│   └── 约束全部？ → smp_mb()（dmb ish）
├── CPU ↔ DMA？
│   ├── 只写？ → wmb()（dsb st）
│   ├── 只读？ → rmb()（dsb ld）
│   └── 全部？ → mb()（dsb sy）
├── 系统寄存器/TLB？ → dsb + isb
└── 能用 LDAR/STLR？ → 优先用（比显式屏障高效）
```

---

## 19.6 HFT 中的屏障使用

```c
// SPSC 无锁队列（Single Producer Single Consumer）
template<typename T, size_t N>
class SPSCQueue {
    T buffer[N];
    std::atomic<size_t> write_idx{0};
    std::atomic<size_t> read_idx{0};

    void push(const T& val) {
        size_t w = write_idx.load(std::memory_order_relaxed);
        buffer[w % N] = val;
        // release: 保证 buffer 写在 write_idx 更新前可见
        write_idx.store(w + 1, std::memory_order_release);
    }

    bool pop(T& val) {
        size_t r = read_idx.load(std::memory_order_relaxed);
        // acquire: 保证读 write_idx 后再读 buffer
        if (r == write_idx.load(std::memory_order_acquire))
            return false;
        val = buffer[r % N];
        read_idx.store(r + 1, std::memory_order_relaxed);
        return true;
    }
};
```

> **HFT 核心**：SPSC 队列用 release/acquire 配对，编译为 STLR/LDAR，比 mutex 快 10-100 倍。

---

## 19.7 易错点清单

1. **屏障加错位置** → 加在临界区外 vs 内，效果完全不同。
2. **屏障类型选错** → DMA 用了 `smp_mb`（不够强，不含 DMA 可见性）。
3. **忘加编译器屏障** → 硬件屏障不阻止编译器重排。
4. **过度使用 `dmb sy`** → 很多场景只需 `ishst`/`ishld`，全屏障性能损失大。
5. **不读内核源码就写裸屏障** → Linux 有完善的屏障 API，优先用。

---

## 书中思考题（自测）

1. 自旋锁获取和释放各需要什么屏障？为什么？
2. 消息传递场景为什么只需要 wmb/rmb 而不需要 mb？
3. DMA 场景为什么必须用 DSB 而不是 DMB？
4. 修改页表后 TLB 维护需要什么屏障序列？
5. HFT 的 SPSC 队列用什么内存序？

**参考答案：**

1. 获取后 `smp_mb`（临界区不重排到锁前）；释放前 `smp_mb`（临界区写在释放前可见）。  
2. 生产者只需 Store-Store 有序（`wmb`）；消费者只需 Load-Load 有序（`rmb`）。不需要 Store-Load 或 Load-Store 约束。  
3. DMB 不停 CPU，`start_dma()` 可能先执行。DSB **完全停住**确保写入完成。  
4. `TLBI` → `DSB ISH`（等刷新完成）→ `ISB`（重新取指）。  
5. 生产者 `release`（STLR）；消费者 `acquire`（LDAR）。

---

上一章 [Ch18 内存屏障](../../chapter-18-memory-barriers/) · 下一章 [Ch20 原子操作](../../chapter-20-atomic-operations/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
