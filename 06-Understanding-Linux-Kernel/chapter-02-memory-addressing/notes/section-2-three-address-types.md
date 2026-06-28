## 2. 三种内存地址与 MMU

> 80x86 上必须区分三种地址；MMU 分两阶段完成翻译

---

### 一、三种地址

| 类型 | 英文 | 是什么 |
|------|------|--------|
| **逻辑地址** | Logical address | 机器指令里的地址；**段 + 偏移量** |
| **线性地址** | Linear / Virtual address | 32 位无符号整数；可寻址 **4 GB** 空间 |
| **物理地址** | Physical address | 内存芯片上的真实地址；送到**内存总线**的电信号 |

---

### 二、MMU 翻译流水线

```
逻辑地址
  ↓ 分段单元（Segmentation）
线性地址
  ↓ 分页单元（Paging）
物理地址
```

- **分段单元：** 查 GDT/LDT 中的段描述符 + 偏移 → 线性地址  
- **分页单元：** 查页目录/页表 → 物理地址；非法访问 → **缺页异常（Page Fault）**

→ 分段详 [section-3](./section-3-segmentation.md) · 分页详 [section-4](./section-4-hardware-paging.md)

---

### 三、和 Ch 1 / 后续章的关系

- Ch 1：**虚拟内存**概念  
- 本章：**地址怎么一步步变成物理 RAM**  
- Ch 8–9：**内核如何用这套机制**管理进程内存

---

← [1. 本章定位](./section-1-chapter-scope.md) · 下一节 [3. 分段机制](./section-3-segmentation.md)
