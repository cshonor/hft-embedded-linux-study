# 3.3 The Out-of-Memory Handler（OOM 处理器）

> 所属：**The Rust Runtime** · [← 章索引](./README.md)

启用 **`alloc`** 后，分配失败须定义行为。

## 策略

| 策略 | 场景 |
|------|------|
| **abort / halt** | 嵌入式硬实时 |
| **返回 `None` / 错误** | 自定义分配器 API |
| **永不失败分配器** | 预分配池（仍须证明上界） |

`std` 下 OOM 默认为 abort；`no_std` + `alloc` 须显式处理 **`handle_alloc_error`**（以当前 Rust 文档为准）。

→ [02 动态分配](./02-dynamic-memory-allocation.md)
