## 4. 缺页异常处理程序 (Page Fault Handler)

> Linux 用户内存管理的 **绝对核心** — `do_page_fault()`

---

### 一、缺页何时发生

CPU 访问某虚拟地址，页表项显示：

- **不存在**（Present 位为 0）  
- 或 **权限不符**（写只读页等）  

→ 触发 **缺页异常**，进入 `do_page_fault()`。

→ 异常入口：[Ch 4 中断与异常](../../chapter-04-interrupts-and-exceptions/)

---

### 二、两条截然不同的路径

`do_page_fault()` 首先要 **分类**：

| 情况 | 处理 |
|------|------|
| **非法访问** | 地址不在任何 VMA 内，或权限不允许（如对只读区写入）→ 通常发 **`SIGSEGV`** |
| **合法缺页** | 地址在合法 VMA 内，但物理页尚未建立 → 进入 **请求调页** / COW 等 |

**栈扩展特例：** 用户栈 **向下增长** 越界时，若仍在允许范围内，调用 **`expand_stack()`** 自动扩展栈 VMA，而非直接 SIGSEGV。

---

### 三、Major Fault vs Minor Fault

| 类型 | 典型场景 | 代价 |
|------|----------|------|
| **Minor Fault** | 页已在 RAM（如共享库已缓存、零页映射），仅需 **建立/更新页表项** | 较低 |
| **Major Fault** | 需从 **磁盘** 读入（可执行文件映射、swap 换入） | 高 — 阻塞 I/O |

HFT 热路径应 **减少 Major Fault**（预加载、`mlock`、大页预 touch）。

→ 页回收 / swap：[Ch 17 页回收](../../chapter-17-page-reclaim.md)

---

### 四、处理流概览

```
缺页异常
    ↓
do_page_fault()
    ↓
查红黑树 → 找到 VMA？
    ├─ 否 → SIGSEGV（或 expand_stack）
    └─ 是 → 权限检查
              ├─ 写共享只读页 → do_wp_page()（COW，见 section-6）
              ├─ 匿名区 → do_anonymous_page()（见 section-5）
              └─ 文件映射 → 读磁盘 / 页缓存
```

### 常见陷阱

1. 把所有 page fault 当错误——demand paging 和 COW 是正常机制，不是错误
2. 混淆 major fault 和 minor fault——major 要读磁盘（慢），minor 在内存中解决（快但仍纳秒级）
3. 在 HFT 热路径触发 page fault——page fault 开销 1-10us，对 HFT 是灾难

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** page fault 的完整处理流程？

<details><summary>答案</summary>

① CPU 触发 #PF，`CR2` = 故障地址。② `do_page_fault()` → `handle_mm_fault()`。③ 查找 VMA（maple tree）：无 VMA → `SIGSEGV`。④ VMA 存在但权限不符（如写只读 VMA）→ `SIGSEGV`。⑤ PTE 不存在 + 匿名页 → `do_anonymous_page()`（分配零页）。⑥ PTE 不存在 + 文件页 → `do_fault()` → `vm_ops->fault()`。⑦ PTE 存在但只读 + 写操作 → `do_wp_page()`（COW）。⑧ 返回 0 = 成功，返回非 0 = `SIGSEGV`/OOM。

</details>

**Q2.** HFT 如何消除热路径上的 page fault？

<details><summary>答案</summary>

① `mlockall(MCL_CURRENT | MCL_FUTURE)`：锁定所有当前页 + 未来映射的页，禁止 swap。② `MAP_POPULATE`：`mmap` 时预建页表，物理页立即分配。③ `memset(buf, 0, size)`：强制触发所有页的 COW/minor fault，之后不再 fault。④ 大页（`MAP_HUGETLB`）：减少 TLB miss + 减少 PTE 数量。⑤ 检查：`perf stat -e page-faults ./hft_engine` 应显示 0 fault。

</details>

**Q3.** major fault 和 minor fault 分别在什么情况下发生？对 HFT 的影响？

<details><summary>答案</summary>

Minor fault：① demand paging（PTE 不存在但物理页在内存，如首次访问匿名页）。② COW（fork 后子进程首次写）。处理时间：~1-5us。Major fault：① 从 swap 读入。② 文件映射页不在 page cache（需磁盘 I/O）。处理时间：~毫秒。HFT 要求两者都为 0：`mlockall` 防 swap，`MAP_POPULATE` + 预 `read()` 填充 page cache。

</details>

</details>

---

← [3. 内存区 VMA](./section-3-内存区VMA.md) · 下一节 [5. 请求调页](./section-5-请求调页.md)
> ↔ [LKD Ch15 §15.7 页表](../../../05-linux-kernel/chapter-15-process-address-space/notes/section-15.7-页表.md)
