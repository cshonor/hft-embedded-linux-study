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

### 常见陷阱

1. 以为现代 x86-64 仍用四级页表——5.x 内核已支持五级页表（PGD→P4D→PUD→PMD→PTE），需 CONFIG_X86_5LEVEL=y
2. 把 `pgd_offset()` 的参数搞反——`pgd_offset(mm, address)` 第一个参数是 `mm_struct`，不是 `task_struct`
3. 以为 `pgd_none()` 返回 true 就代表这段地址没被映射——也可能是被 `PROT_NONE` 保护的页，需要看 `pmd_present()` 进一步判断

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 四级页表和五级页表在代码层面有什么区别？

<details><summary>答案</summary>

五级页表多了一层 P4D。内核用 `pgtable-nopud.h`/`pgtable-nop4d.h` 等头文件在编译时折叠不存在的层级，使四级硬件在软件层面仍呈现五级接口。`CONFIG_X86_5LEVEL=y` 时 P4D 真实存在于硬件页表中。

</details>

**Q2.** `pgd_offset(mm, addr)` 返回什么？怎么进一步拿到 PTE？

<details><summary>答案</summary>

返回 `pgd_t*` 指针。链路：`pgd_offset(mm, addr)` → `p4d_offset(pgd, addr)` → `pud_offset(p4d, addr)` → `pmd_offset(pud, addr)` → `pte_offset_map(pmd, addr)`。每一步都要检查 `*_none()` 或 `*_present()`。

</details>

**Q3.** 为什么内核要把三级/四级/五级页表统一成五级软件接口？

<details><summary>答案</summary>

可移植性。不同架构页表级数不同（ARM64 可配 3/4/5 级，x86-64 可配 4/5 级）。统一成五级接口后，通用代码不用 `#ifdef` 区分级数——不存在的层会被折叠成「transparent」操作（直接传递指针）。

</details>

</details>

---

← [4. 硬件分页](./section-4-硬件分页.md) · 下一节 [6. 内存布局与 TLB](./section-6-内存布局与TLB.md)
