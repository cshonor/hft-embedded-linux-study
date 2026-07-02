## ④ kmalloc() 与 kfree()

**按字节分配** — 类似用户态 `malloc`，但保证 **物理连续**。

```c
ptr = kmalloc(size, GFP_KERNEL);
kfree(ptr);
```

#### gfp_mask 常用标志

| 标志 | 行为 | 使用上下文 |
|------|------|------------|
| **`GFP_KERNEL`** | 常规；**可睡眠** 腾内存 | **进程上下文** — 成功率高 |
| **`GFP_ATOMIC`** | **绝不睡眠** | **中断、下半部、持 spinlock** — 紧张时 **易失败** |
| **`GFP_DMA`** | 从 **ZONE_DMA** 分配 | DMA 缓冲区 |

| 对比 Ch 5/10 | |
|--------------|--|
| syscall 里大块分配 | 常 `GFP_KERNEL` |
| ISR 里小缓冲 | **`GFP_ATOMIC`** + 短小 |

**HFT 用户态镜像：** 热路径 **预分配池** ≈ 避免 tick 内 `GFP_ATOMIC` 失败。

→ [01-CSAPP Ch9 malloc/池化](../../../../01-CSAPP-3rd/chapter-09-virtual-memory/)

---
