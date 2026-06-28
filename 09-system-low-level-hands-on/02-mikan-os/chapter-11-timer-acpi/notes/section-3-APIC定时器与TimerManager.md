## 3. APIC 定时器中断与 TimerManager

---

### 一、从轮询到 Periodic 中断

**Ch9：** 读 APIC **Current Count** 差值 — **主动查询**。

**Ch11：** 配置 Local APIC Timer 为 **循环（Periodic）模式**：

```
计数递减 → 0 → 触发中断（向量 0x41）→ 自动重装 → 循环
```

| 配置项 | 说明 |
|--------|------|
| **中断向量** | **0x41**（示例 — 在 IDT 注册 ISR） |
| **分频 / 初值** | 调节 **中断周期** — 目标 ~**1ms** |
| **模式** | **Periodic** — 非 one-shot |

**CPU 不再：** `hlt` + 循环查计数 — **ISR 推 tick**。

→ [Ch7 IDT/ISR](../chapter-07-interrupt-fifo/notes/section-2-中断处理程序与EOI.md)

---

### 二、TimerManager 与 tick_

```cpp
class TimerManager {
    volatile uint64_t tick_;  // 必须 volatile
public:
    void IncrementTick();     // ISR 调用
    uint64_t GetTick() const;
};
```

**ISR（示意）：**

```cpp
__attribute__((interrupt))
void TimerHandler() {
    timer_manager.IncrementTick();
    NotifyEndOfInterrupt();
}
```

---

### 三、为何 `volatile`

| 无 volatile | 有 volatile |
|-------------|-------------|
| 编译器见 main 循环 `while (tick == old)` **认为 tick 不变** | 强制 **每次从内存读** |
| **优化掉 reload** → **死循环** | ISR 更新 **对 main 可见** |

**规则：** **ISR 写 · 主循环读** 的共享变量 — **几乎总是 volatile**（更后可用 atomic）。

→ 与 [03 SysPerf 编译器优化](../../../03-Systems-Performance-2nd/) 相关

---

### 四、~1ms 分辨率

缩短 APIC 中断周期 → **tick 约 1ms** — 足够：

- 控制台 **光标闪烁**
- 应用 **sleep 超时**
- Ch13 **时间片** 调度（预告）

**注意：** 校准前 tick **不是真实毫秒** — §5 ACPI 校准。

---

← [2. 重构](./section-2-源码重构.md) · 下一节 [4. 多定时器](./section-4-优先级队列与多定时器.md)
