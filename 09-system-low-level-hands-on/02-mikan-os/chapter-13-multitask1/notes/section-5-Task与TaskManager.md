## 5. Task 与 TaskManager

---

### 一、Task 类

**代表单个可调度执行流：**

| 成员 | 说明 |
|------|------|
| **`id_`** | 唯一 ID |
| **`stack_`** | **独立栈** 数组（如 64KB） |
| **`context_`** | **TaskContext** — 初始 RSP 指向栈顶，RIP 指向 **Task 入口函数** |
| **入口** | `void TaskFunc()` — 任务主体循环 |

**创建时初始化 context：**

```
ctx.rsp = stack + sizeof(stack)
ctx.rip = &TaskEntryTrampoline
```

---

### 二、TaskManager 类

**统筹调度：**

| 职责 | 实现 |
|------|------|
| **持有所有 Task** | **`std::vector<Task*>`** 或 `vector<Task>` |
| **`AddTask`** | 构造 Task · push_back |
| **`Switch`** | `current_index_ = (current+1) % size` · **SwitchContext** |
| **`CurrentTask`** | 供 syscall/消息 查询 |

**摆脱硬编码两个 Task** — **动态数量**。

→ [Ch9 sbrk/new + STL](../chapter-09-layers/notes/section-2-sbrk与new运算符.md)

---

### 三、典型 Task 划分（书中）

| Task | 职责 |
|------|------|
| **Main / GUI** | 鼠标、键盘、绘制 |
| **Idle / Demo** | 空转计数等 — 占时间片 |

**问题：** 二者 **同等 20ms** → §6 鼠标卡。

---

← [4. 抢占](./section-4-抢占式多任务与时间片.md) · 下一节 [6. 小结](./section-6-均分时间片问题与小结.md)
