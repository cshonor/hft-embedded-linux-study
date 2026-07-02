## 5. sort、cat 优化与终端修复

---

### 一、sort 应用

**按行排序 — 管道下游典型消费者：**

```
grep pattern file | sort
cat memmap | grep hoge | sort
```

```cpp
// 读 stdin 至 EOF(Ctrl+D) 或 pipe 关闭
lines = read_all_lines(stdin);
std::sort(lines.begin(), lines.end());
for (auto& l : lines) write(stdout, l);
```

| 展示 | **小工具组合 = 复杂处理** |
|------|---------------------------|

→ [Ch25 grep](../chapter-25-app-read-file/notes/section-6-readfile-grep与小结.md)

---

### 二、cat 优化：ReadDelim 按行

**旧 cat：** 大块 read — **管道场景** 右端 **等整块** — 慢。

```cpp
while (ReadDelim(fd, '\n', line)) {
    PrintToFD(STDOUT_FILENO, line.c_str());
    PrintToFD(STDOUT_FILENO, "\n");
}
```

| 效果 | 左 **每行即 flush 到 Pipe** — 右 **grep/sort 早开工** |

---

### 三、终端 Redraw Bug

**多级管道测试：** 终端 **仅最后一行更新** · **不 Scroll**。

**修复：** 每次 **PrintToFD / Echo** 后：

```cpp
Terminal::Redraw();   // 强制 shadow → Layer 合成
```

| 根因 | 管道 **高频小块输出** 未 **触发 kLayer** |
|------|------------------------------------------|

→ [Ch22 NO_REDRAW](../chapter-22-graphics-events1/notes/section-4-性能测量与批量重绘.md) · [Ch15 DrawArea](../chapter-15-terminal/)

---

← [4. WaitFinish](./section-4-WaitFinish与任务同步.md) · 下一节 [6. 共享内存](./section-6-共享内存与小结.md)
