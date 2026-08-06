# 5.2 六种内存序

> 第 5 章 · 上一节：[5.1 内存模型基础](01-memory-model-basics.md) · 下一节：[5.3 atomic 操作](03-atomic-ops.md)

## 这节讲什么

六种内存序的语义与代价——从最弱的 `relaxed` 到最强的 `seq_cst`。选择正确的内存序是无锁编程的核心技能。

---

## 六种内存序

```cpp
enum memory_order {
    relaxed,     // 无同步，仅原子性
    consume,     // 数据依赖（实践中几乎等同 acquire，已弃用倾向）
    acquire,     // 读：之后的读写不能重排到之前
    release,     // 写：之前的读写不能重排到之后
    acq_rel,     // 读写都有：RMW 操作用
    seq_cst      // 全局总序，最强，默认
};
```

| 内存序 | 代价 | 典型用法 |
|--------|------|----------|
| `relaxed` | 最低 | 计数器、无依赖的状态标志 |
| `acquire`/`release` | 中 | 配对使用，构建 happens-before |
| `seq_cst` | 最高 | 默认，简单但可能成为瓶颈 |

### 关键直觉

- `release` 写 + `acquire` 读配对：写线程在 release 前的所有写，对读到该值的读线程可见
- `seq_cst` 额外保证**全局总序**——所有线程看到的操作顺序一致
- `relaxed` 只保证原子变量本身的原子性，不提供跨变量同步

---

## 新手要点

- **默认 `seq_cst` 最安全**：新手用 `seq_cst`（默认）不会错，只是慢。
- **优化用 acquire/release**：理解 happens-before 后，配对 acquire/release 在大多数场景足够且更快。
- **别用 `consume`**：实践中几乎等同 acquire，标准委员会在讨论废弃。别碰。

---

## HFT 关联

- **`seq_cst` 是热路径性能杀手**：需要 CPU 全局内存屏障（x86 上 `mfence`/`lock` 前缀），比 relaxed 慢数倍。热路径尽量用 acquire/release。
- **x86 的 TSO 优势**：x86 是 TSO（Total Store Order），acquire/load 和 release/store 几乎免费。ARM 是弱内存序，acquire/release 也有显式屏障——跨平台无锁代码要测 ARM。

---

## 自测题

1. 六种内存序中，为什么 `seq_cst` 是默认但热路径要换成 acquire/release？代价差在哪？
2. `relaxed` 保证了什么？不保证什么？
3. `release` 写 + `acquire` 读如何建立 happens-before？
4. x86 的 TSO 对无锁编程有什么好处？

---

## 参考与延伸

- 下一节：[5.3 atomic 操作](03-atomic-ops.md)
- 回到：[第 5 章](README.md)
