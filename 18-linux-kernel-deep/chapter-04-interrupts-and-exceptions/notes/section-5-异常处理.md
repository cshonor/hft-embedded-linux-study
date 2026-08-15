## 5. 异常处理 (Exception Handling)

---

### 一、通用流程

1. 在内核栈 **保存寄存器**  
2. 调用高级 **C 语言** 处理函数  
3. 处理完毕 **退出**（Fault 可能返回原指令重试）

---

### 二、用户态异常 → Unix 信号

大多数用户态异常被当作 **错误**，处理程序向进程发 **信号**：

| 异常 | 典型信号 |
|------|----------|
| 除以零 | `SIGFPE` |
| 无效内存访问 | `SIGSEGV` |
| 非法指令 | `SIGILL` |

→ 信号机制：[Ch 11 信号](../../chapter-11-signals.md)

---

### 三、内核态异常 → Kernel Oops

若异常发生在 **内核态**，且由 **内核 bug** 引起：

1. 打印寄存器 + 内核栈快照 — 著名的 **"Kernel oops"**  
2. **强制终止** 当前上下文，防止数据损坏  

生产环境 oops 常意味着驱动或内核模块 bug。

---

### 四、缺页异常（特殊 Fault）

缺页是 **可纠正 Fault** 的成功案例 — 分配页、COW、swap 等，纠正后程序继续。

→ [Ch 8 内存管理](../../chapter-08-memory-management.md) · [Ch 9 地址空间](../../chapter-09-process-address-space.md)

### 常见陷阱

1. 以为所有异常都走 `do_page_fault()`——只有 #PF（异常 14）走缺页路径，#GP/#UD 等走各自的 handler
2. 混淆缺页异常的「minor fault」和「major fault」——minor 是 COW/demand paging（内存中解决），major 要读磁盘
3. 在异常处理中做复杂操作——异常处理应尽量简单，复杂逻辑交给上层或下半部

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** page fault handler 的主要判断流程是什么？

<details><summary>答案</summary>

① 读 `CR2` 获取故障地址。② 在 `current->mm` 的 VMA 中查找（现代内核用 maple tree）。③ 找到 VMA → 检查权限（读/写/执行 vs VMA flags）。④ 权限 OK → demand paging 或 COW。⑤ 无 VMA 或权限不符 → `SIGSEGV`。⑥ 内核态 fault → `fixup_exception()` 搜索异常表，找到 fixup 地址则跳转，否则 `die()` / panic。

</details>

**Q2.** minor fault 和 major fault 在 HFT 中分别意味着什么？

<details><summary>答案</summary>

Minor fault：页在内存但 PTE 不存在（demand paging/COW），处理时间 ~1-5 us。Major fault：页需从磁盘/swap 读入，处理时间 ~毫秒级。HFT 热路径两种都不能有：`mlockall(MCL_CURRENT | MCL_FUTURE)` 锁定内存防 swap，预分配 + `MAP_POPULATE` 预填充页表消除 demand paging。

</details>

**Q3.** 内核态访问用户态指针触发 page fault 时怎么处理？

<details><summary>答案</summary>

`copy_from_user()` 等 API 在异常表中注册了 fixup entry。如果用户指针触发 #PF 且 fault 不可恢复（如无 VMA），`do_page_fault()` → `fixup_exception()` 找到对应 fixup → 跳到 `copy_from_user` 的错误返回点 → 返回 `-EFAULT`。这就是为什么内核用 `copy_from_user()` 而非直接解引用用户指针。

</details>

</details>

---

← [4. 控制路径嵌套](./section-4-控制路径嵌套.md) · 下一节 [6. I/O 中断处理](./section-6-IO中断处理.md)
