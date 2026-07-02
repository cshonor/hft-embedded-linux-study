## 5. FIFO 与 ArrayQueue

---

### 一、为何 ISR 里不能慢慢画图

若 ISR 内 **擦除/重绘整个光标**：

| 问题 | 后果 |
|------|------|
| 执行 **毫秒级** | 新 USB 事件 **堆积/丢失** |
| 嵌套中断复杂 | 帧缓冲 **重入冲突** |

**策略（Linux 顶半部/底半部雏形）：**

```
ISR（顶半部）：  读硬件 → 构造 Message → Push 队列 → EOI
主循环（底半部）： Pop Message → 更新光标/绘图
```

---

### 二、Message 与定长 FIFO

**Message** — 描述一次输入事件（如鼠标 **Δx, Δy**、按钮）。

**`ArrayQueue<T, N>`** — C++ **模板** 手写 **环形定长队列**：

```cpp
template <class T, size_t N>
class ArrayQueue {
    T data_[N];
    size_t head_, tail_, count_;
public:
    bool Push(const T& m);   // 满则丢弃或返回 false
    bool Pop(T& m);
    bool Empty() const;
};
```

| 设计 | 说明 |
|------|------|
| **定长数组** | 无 **malloc** — 符合 Ch8 前 **无堆** 环境 |
| **模板** | 类型安全 · 编译期大小 |
| **O(1) Push/Pop** | 适合 ISR |

→ [appendix-D C++ 模板](../../appendix-D-cpp-templates/)

---

### 三、与 01 FIFO 对照

01 **Day 7–8** 同样用 **FIFO** 解耦中断与主循环 — MikanOS 在 **64 位 + MSI + USB** 语境下 **复现同一架构模式**。

→ [01 Day 7 FIFO](../../02-30days-os/day-07-fifo/)

---

← [4. MSI](./section-4-MSI中断配置.md) · 下一节 [6. 事件循环](./section-6-事件循环与并发控制.md)
