## 4. 栈扩容与 FreeType Bug

---

### 一、现象

**集成 FreeType 显示日文 → 系统强制重启**（三重 fault / 栈破坏）。

| 怀疑 | 验证 |
|------|------|
| **堆不足** | 非主因 — **栈溢出** |
| **FreeType 深调用链** | **局部数组/递归** 吃栈 |

---

### 二、根因：4KiB 栈过小

**Ch13–24 默认：**

| 栈 | 原大小 |
|----|--------|
| **任务栈**（内核 Task） | **4KiB** |
| **应用用户栈** | **4KiB** |

**FreeType `FT_Load_Glyph` 等** — 单路径 **>4KiB** — **覆盖返回地址**。

---

### 三、修复：扩容至 32KiB

```cpp
constexpr size_t kTaskStackBytes = 32 * 1024;
constexpr size_t kAppStackBytes  = 32 * 1024;
```

| 对象 | 调整 |
|------|------|
| **TaskTerminal / Main …** | 分配 **32KiB 内核栈** |
| **CallApp** | 用户 **RSP 区间 32KiB** · **User 页表映射** |

**教训：** 引入 **第三方 C 库进内核** — **必须评估栈/堆** — 与 **Linux 内核 stack size 配置** 同理。

→ [Ch21 IST/RSP0](../chapter-21-window-apps/notes/section-2-IST与定时器中断栈修复.md) · [Ch20 TSS](../chapter-20-syscall/notes/section-3-TSS与RSP0内核栈.md)

---

← [3. FreeType](./section-3-FreeType与矢量字体.md) · 下一节 [5. 重定向](./section-5-标准输出重定向.md)
