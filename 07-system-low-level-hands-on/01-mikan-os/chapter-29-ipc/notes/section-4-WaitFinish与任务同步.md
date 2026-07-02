## 4. WaitFinish 与任务同步

---

### 一、为何需要 WaitFinish

**管道：** 主终端 **启动左+右** — 必须 **等右端结束** 才：

- 恢复 **提示符 `>`**
- 设置 **`last_exit_code_`**
- **关闭 PipeDescriptor**

---

### 二、Finish / WaitFinish

```cpp
class Task {
    bool finished_;
    int exit_code_;
public:
    void Finish(int code) {
        finished_ = true;
        exit_code_ = code;
        WakeupWaiters();
    }
    int WaitFinish(TaskId child) {
        SetWaiting();
        until (child.finished_) schedule;
        return child.exit_code_;
    }
};
```

| 调用点 | 说明 |
|--------|------|
| **右端 exit/KillApp** | **Finish(code)** |
| **主 Terminal** | **WaitFinish(right_task)** |
| **多级管道** | 可 **链式等待** 最右端 |

→ [Ch14 Wakeup](../chapter-14-multitask2/notes/section-3-Sleep与Wakeup.md)

---

### 三、终端任务优雅关闭

**Ch29 新增：** 隐藏 **右端 Terminal** 在 **完成后**：

```
CloseWindow (若曾开) · 从 TaskManager 移除 · 释放 Pipe
```

**避免：** 僵尸 **Terminal Task** · **msgs_ 泄漏**。

---

### 四、键盘 `|` 支持

**Ch12 键盘表扩展** — **Shift+\\** 等映射 **`|`** — 否则 **无法输入管道命令**。

→ [Ch12 键盘](../chapter-12-keyboard/)

---

← [3. 管道](./section-3-管道机制与PipeDescriptor.md) · 下一节 [5. sort/cat](./section-5-sort-cat优化与终端修复.md)
