## 3. more 命令与管道按键

---

### 一、more — 分页浏览

**长输出超出终端客户区时：**

```
> ls apps | more
```

| 行为 | 说明 |
|------|------|
| **满屏暂停** | 等 **空格/Enter** 翻页 |
| **q 退出** | 结束 **more** |

**实现：** **ReadLine/ReadChunk from stdin** · **PrintToFD** 按 **行/屏高** 输出 · **ReadEvent 等键**。

→ [Ch29 管道](../chapter-29-ipc/notes/section-3-管道机制与PipeDescriptor.md)

---

### 二、管道 + more 的按键路由 Bug

**问题：** **`ls | more`** — 按键送到 **左端 Terminal** · **more 收不到**。

**修复：** 管道活跃时 **ActiveLayer / 事件路由** 指向 **管道右端 Task**（消费 stdin 者）：

```cpp
if (pipe_active) {
    PostKeyEvent(pipe_right_task, ev);
} else {
    PostKeyEvent(active_terminal, ev);
}
```

| 原则 | **谁 Read(0) 谁收键** |
|------|----------------------|

→ [Ch23 键盘事件](../chapter-23-graphics-events2/notes/section-6-键盘升级与blocks游戏.md) · [Ch29 WaitFinish](../chapter-29-ipc/notes/section-4-WaitFinish与任务同步.md)

---

← [2. PATH](./section-2-FindCommand与PATH搜索.md) · 下一节 [4. cat](./section-4-cat标准输入与重定向建文件.md)
