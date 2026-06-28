## 4. 硬件分页机制

> 分页单元：线性地址 → 物理地址；非法访问 → 缺页异常

---

### 一、常规分页（80x86 默认）

| 组件 | 作用 |
|------|------|
| **页目录（Page Directory）** | 第一级索引 |
| **页表（Page Table）** | 第二级索引 |
| **页框（Page Frame）** | 通常 **4 KB** 一页 |

**两级分页** → 32 位线性地址拆成目录索引 + 表索引 + 页内偏移。

---

### 二、扩展分页（Extended Paging）

- **省略页表层级**，直接用 **4 MB 大页**
- 减少 TLB 压力，适合映射大块连续区域（如内核代码）

---

### 三、PAE（Physical Address Extension）

32 位地址引脚默认只能直接寻址 **4 GB RAM**。PAE：

| 特性 | 说明 |
|------|------|
| **36 位物理地址** | 最多 **64 GB** RAM |
| **三级分页** | 在常规两级之上扩展一层 |

→ Linux 如何在统一框架里适配 → [section-5](./section-5-Linux四级分页.md)

---

### 四、加速地址翻译

| 机制 | 作用 |
|------|------|
| **Hardware Cache** | CPU 缓存最近访问的数据/指令 |
| **TLB（Translation Lookaside Buffer）** | 缓存**页表项**，避免每次访问都 walk 页表 |

TLB  miss 时才慢路径查内存中的页表。Linux 如何刷新 TLB → [section-6](./section-6-内存布局与TLB.md)

---

### 五、缺页异常

访问未映射或权限不对的线性地址 → **Page Fault** → 内核处理（分配物理页、COW、swap 等）

→ 深潜：[Ch 8 内存管理](../../chapter-08-memory-management.md) · [Ch 9 进程地址空间](../../chapter-09-process-address-space.md) · [Ch 17 页回收](../../chapter-17-page-reclaim.md)

---

← [3. 分段](./section-3-分段机制.md) · 下一节 [5. Linux 四级分页](./section-5-Linux四级分页.md)
