## 3. 系统调用处理程序与服务例程

---

### 一、命名约定

| 用户可见 | 内核服务例程 |
|----------|--------------|
| `xyz()` syscall | **`sys_xyz()`** |

例：`read()` → `sys_read()`，`brk()` → `sys_brk()`。

---

### 二、系统调用分派表 `sys_call_table`

内核用 **函数指针数组** 将 **系统调用号** 映射到服务例程：

| 要素 | 2.6 典型值 |
|------|------------|
| 表名 | **`sys_call_table[]`** |
| 容量 | **`NR_syscalls`**（如 289） |
| 调用号寄存器 | **`eax`**（x86） |

**分派逻辑（概念）：**

```
nr = eax
handler = sys_call_table[nr]   // 2.6: nr * 4 + 表基址
ret = handler(...)
```

非法调用号 → 返回错误。

---

### 三、与前后章的衔接

| syscall | 内核实现章节 |
|---------|--------------|
| `fork` / `exit` / `wait` | [Ch 3](../../chapter-03-processes/notes/section-6-创建与销毁.md) |
| `brk` / `mmap` | [Ch 9](../../chapter-09-process-address-space/) |
| `nice` / `sched_setscheduler` | [Ch 7](../../chapter-07-process-scheduling/notes/section-6-调度相关系统调用.md) |
| `gettimeofday` 等 | [Ch 6](../../chapter-06-timing/notes/section-6-定时相关系统调用.md) |

本章讲 **如何到达** `sys_*()`，各章讲 **里面做什么**。

---

← [2. POSIX API](./section-2-POSIX-API与系统调用.md) · 下一节 [4. 进入与退出](./section-4-进入与退出.md)
