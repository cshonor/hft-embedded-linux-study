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

---

← [3. 内存区 VMA](./section-3-内存区VMA.md) · 下一节 [5. 请求调页](./section-5-请求调页.md)
