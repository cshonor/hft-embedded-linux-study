## 3. 管道机制与 PipeDescriptor

---

### 一、管道语义

```
cat memmap | grep hoge
    ↑           ↑
  stdout      stdin
  (fd=1)      (fd=0)
```

**目标：** 左 **Write(1,…)** 数据 **流入** 右 **Read(0,…)** — **无需临时文件**。

---

### 二、左右并发策略

| 方案 | 本书 |
|------|------|
| 左 **全跑完** 再右 | 占 **大缓冲** |
| **同时运行** · 小块传递 | **省内存** · **低延迟** |

```
Left Task  write chunk → Pipe → Right Task read chunk
(并行调度 · 任一侧阻塞则 Sleep)
```

→ [Ch14 Sleep/Wakeup](../chapter-14-multitask2/)

---

### 三、解析 `|`

**ExecuteLine 分词扩展（与 Ch28 `>` 类似）：**

```cpp
// "cat memmap | grep hoge"
auto [left_line, right_line] = SplitPipe(line);
SpawnHiddenTerminal(right_line);     // 右端 · show_window=false
SetupPipe(left_task, right_task);
RunLeftCommand(left_line);           // 左端在当前终端或并行 Task
```

| 右端 | **隐藏终端任务**（Ch24 **noterm** 思路） |
|------|----------------------------------------|
| **键盘 `\|`** | 驱动 **HID 映射** 补充 **\|** 键码 |

→ [Ch24 noterm](../chapter-24-multi-terminal/notes/section-4-窗口层级Bug与noterm.md)

---

### 四、PipeDescriptor 类

**继承 Ch26 FileDescriptor：**

```cpp
class PipeDescriptor : public FileDescriptor {
    TaskId peer_task_;
public:
    size_t Write(const void* buf, size_t len) override {
        SendMessage(peer_task_, Message::kPipe, buf, len);
        return len;
    }
    size_t Read(void* buf, size_t len) override {
        WaitMessage(kPipe, buf, len);
        return got;
    }
};
```

| 绑定 | 说明 |
|------|------|
| **左 files_[1]** | **PipeDescriptor** → **右 Task** |
| **右 files_[0]** | **配对接收端**（或对称 Pipe 对） |
| **传输** | **Task::msgs_ 队列** · **Message::kPipe** |

**非内核环形 buffer syscall pipe(2)** — **消息队列模拟** — 教学 **清晰**。

→ [08 TLPI pipe](../../../06-The-Linux-Programming-Interface/)

---

← [2. $?](./section-2-退出码与echo-question.md) · 下一节 [4. WaitFinish](./section-4-WaitFinish与任务同步.md)
