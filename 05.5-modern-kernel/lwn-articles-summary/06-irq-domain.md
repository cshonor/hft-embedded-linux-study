# IRQ Domain 框架 — 中断号映射

> **原文:** [IRQ domain framework](https://lwn.net/Articles/460160/) (LWN, 2011)
> **内核版本:** 3.x+ (引入), 6.x (广泛使用)
> **对标旧书:** ULK3 Ch4 (硬编码中断号)

---

## 核心观点

ULK3 时代中断号是全局硬编码的（如 IRQ 0 = timer, IRQ 4 = COM1）。现代内核用 IRQ Domain 框架实现**硬件中断号 → Linux IRQ 号**的动态映射，支持设备树和 ACPI 中断描述。

### 旧模型的问题

```
// ULK3 时代 — 全局 IRQ 号
irq_desc[32]  // 全局数组，直接索引
request_irq(4, com1_handler, ...)  // IRQ 4 = COM1
```

问题：
- 多个中断控制器（GIC + GPIO 控制器 + PCIe MSI）的硬件中断号会重叠
- 不支持设备树动态描述中断
- 扩展性差

### IRQ Domain 模型

```c
// 每个中断控制器创建一个 irq_domain
struct irq_domain {
    struct irq_domain_ops *ops;  // 映射回调
    int hwirq_max;               // 最大硬件中断号
    // ... 映射表 (linear / radix tree / legacy)
};

// 设备树描述中断时，内核自动创建映射
// hwirq (硬件中断号) → virq (Linux 虚拟 IRQ 号)
unsigned int irq = irq_create_mapping(domain, hwirq);
```

### 三种映射策略

| 策略 | 说明 | 适用场景 |
|------|------|---------|
| **Linear** | 直接数组 `virq[hwirq]` | 硬件中断号范围小且密集 (如 GIC) |
| **Radix Tree** | 基数树映射 | 硬件中断号范围大或稀疏 (如 MSI-X) |
| **Legacy** | 固定映射兼容旧代码 | x86 传统 ISA 中断 |

### 关键 API

```c
// 中断控制器驱动注册 irq_domain
struct irq_domain *domain = irq_domain_add_linear(
    node, 32, &gic_irq_domain_ops, gic);

// 设备驱动请求中断 (通过设备树)
int irq = platform_get_irq(pdev, 0);  // 自动解析设备树
request_irq(irq, my_handler, 0, "my_dev", dev);

// 中断发生时
// 硬件中断 → GIC → hwirq → irq_domain 映射 → virq → handler
```

---

## 与旧书差异

| ULK3 讲的 | 6.x 现代实现 |
|-----------|-------------|
| 全局 `irq_desc[]` 数组 | per-domain 映射 + 全局 `irq_desc[]` |
| 硬编码 IRQ 号 | 动态分配 virq |
| `request_irq(4, ...)` | `platform_get_irq()` + `request_irq()` |
| 无设备树中断解析 | 设备树 `interrupts` 属性自动解析 |
| 不支持 MSI-X 多向量 | irq_domain + radix tree 支持 |

---

## HFT 关联

| 场景 | IRQ Domain 影响 |
|------|----------------|
| **网卡中断绑核** | `irq_set_affinity()` 需要 virq，IRQ Domain 提供映射 |
| **自定义 PCIe 设备** | MSI-X 中断通过 irq_domain 分配 virq |
| **设备树配置** | 树莓派 5 通过设备树描述中断，IRQ Domain 自动映射 |
| **减少中断延迟** | 理解 hwirq → virq 映射有助于排查中断延迟问题 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么不能直接用硬件中断号 (hwirq) 作为 Linux IRQ 号？

> 多个中断控制器可能有相同的 hwirq。例如 GIC 的 hwirq 29 和 GPIO 控制器的 hwirq 29 是不同的中断。IRQ Domain 为每个控制器创建独立的映射空间，将 hwirq 映射到全局唯一的 virq。

**Q2:** Linear 映射和 Radix Tree 映射各适合什么场景？

> Linear 映射用数组直接索引，O(1) 查找，适合硬件中断号范围小且密集（如 GIC 的 0-31 或 0-1020）。Radix Tree 适合硬件中断号范围大或稀疏（如 MSI-X 可能有数千个向量但只用其中一部分），内存效率更高。

**Q3:** 设备树中的 `interrupts = <0 23 4>` 是什么意思？内核如何处理？

> 这是 GIC 的设备树中断描述：`<中断类型 中断号 触发类型>`，即 SPI 中断 23，边沿触发。内核解析设备树时，通过 IRQ Domain 的 `xlate` 回调将设备树描述翻译为 hwirq，然后 `irq_create_mapping()` 分配 virq。

</details>
