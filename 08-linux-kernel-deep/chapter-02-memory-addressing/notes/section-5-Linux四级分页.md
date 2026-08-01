## 5. Linux 四级分页模型

> Linux 2.6 用**统一四级命名**，兼容 32 位两级/三级与 64 位多级硬件

---

### 一、四级结构（自顶向下）

| 层级 | 英文缩写 | 说明 |
|------|----------|------|
| 1 | **PGD** Page Global Directory | 页全局目录 |
| 2 | **PUD** Page Upper Directory | 页上级目录 |
| 3 | **PMD** Page Middle Directory | 页中间目录 |
| 4 | **PT** Page Table | 页表 |

线性地址逐级索引 → 最终指向 **页框** + 页内偏移。

---

### 二、「折叠」：32 位上的技巧

32 位 CPU 硬件可能只有 **两级或三级** 分页，Linux 在代码里仍保留 PGD/PUD/PMD/PT 四层接口，把用不到的层 **fold（折叠）** 掉：

- **同一套源码** 跑在 32 位和 64 位
- 读源码时看到 `pud_none()` / `pmd_offset()` 等，需结合架构看哪几层是空操作

---

### 三、和硬件分页的对应

| 硬件 | Linux 抽象 |
|------|-------------|
| 80x86 两级 | PGD + PT（PUD/PMD 折叠） |
| PAE 三级 | 多一层不折叠 |
| x86-64 四级+ | 四层真正用起来 |

→ 进程页表布局 [section-6](./section-6-内存布局与TLB.md) · 分配物理页 [Ch 8](../../chapter-08-memory-management.md)

---

← [4. 硬件分页](./section-4-硬件分页.md) · 下一节 [6. 内存布局与 TLB](./section-6-内存布局与TLB.md)
