## Day 17 · 命令行窗口

> **原书第十七章** · **Console 雏形** — **idle 任务**、**`console_task`**、Tab 焦点、**每任务 FIFO**、Shift/CapsLock、**键盘 LED**。  
> ← [Day 16](./day-16-多任务2.md) · [08-1 导读](../README.md) · → [Day 18](./day-18-dir命令.md)

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 闲置 + Console** | **`console_task`** 独立任务 | **Idle** 最低优先级 |
| **② Tab 焦点** | **`key_to`** | 标题栏 **灰/亮** 切换 |
| **③ 字符输入** | **TASK 内 FIFO** | Backspace **= 8** |
| **④ Shift/Caps** | **`keytable0/1`** · **`key_shift`** | 符号 + 大小写 **`±0x20`** |
| **⑤ LED** | 端口 **`0x60`** · **`0xED`** | Caps/Num/Scroll 灯 |

---

### ① 闲置任务 · 创建命令行窗口

#### 为何独立 `console_task`？

命令行逻辑 **不应堆进 `HariMain`** — 与 Day 16 **`task_alloc/task_run`** 一致：

```
console_task = task_alloc();
/* 入口 console_main，独立栈、LEVEL… */
task_run(console_task, …);
```

**Console 与 B0～B2 一样后台并发** — 多任务架构 **自然延伸**。

#### Idle Task（闲置任务）

| 概念 | 说明 |
|------|------|
| **何时跑** | **无其他 runnable 任务** 时 |
| **优先级** | **最低 LEVEL** — 纯 **HLT 或空转** 占位 |
| **目的** | 调度器 **总有合法切换目标**；避免 **无任务可跑** 的边界 |

→ [Day 16 sleep/wake/LEVEL](./day-16-多任务2.md)

---

### ② Tab 键 · 切换输入窗口

多窗口 → 按键必须 **发给有焦点的那个**。

| 变量 | 作用 |
|------|------|
| **`key_to`** | 当前 **键盘输入目标**（主窗 / Console 等） |

**Tab：** 在 **主窗口 ↔ 命令行窗口** 间 **轮转焦点**。

**视觉反馈（非活动 / 活动）：**

| 状态 | 标题栏 |
|------|--------|
| **非焦点** | **变灰** |
| **焦点** | **高亮**（原色） |

→ Day 11 **`make_window8` 标题栏** · 现代 WM **focus 指示** 同源

---

### ③ 字符输入 · 退格 · 每任务 FIFO

Day 13 **全局统一 FIFO** 收键鼠；Day 17 **输入投递** 要 **按任务分流**。

#### FIFO 绑在 `struct TASK`

```c
/* 示意 */
struct TASK {
    struct FIFO32 fifo;  /* 该任务私有输入队列 */
    …
};
```

**路径：**

```
键盘中断 → 全局/主路径收 scancode
    → keytable 转字符
    → Put 进 **key_to 指向任务的 fifo**
console_task / HariMain 从 **自己的 fifo Get** 处理
```

#### Console 能力

| 功能 | 实现要点 |
|------|----------|
| **收字存缓冲** | 从 **本任务 fifo** 读 |
| **显示** | 自绘字符（Day 5/14） |
| **Backspace** | 字符码 **8** — 删上一字符 |

**解耦：** 主程序 **只负责路由**；Console **只读自己的 fifo** — 像 **多终端各读各的 stdin**。

→ [Day 14 keytable](./day-14-高分辨率及键盘输入.md) · [Day 13 FIFO 编码段](./day-13-定时器2.md)

---

### ④ Shift 与 CapsLock · 符号与大小写

#### Shift + 双表

| 变量/表 | 作用 |
|---------|------|
| **`key_shift`** | 左/右 Shift **按下状态** |
| **`keytable0`** | 无 Shift |
| **`keytable1`** | 有 Shift → **`!` `%` 等** |

```
scancode → (shift ? keytable1 : keytable0)[code]
```

#### CapsLock + ASCII `0x20`

字母 **大小写 ASCII 差 `0x32`（即 0x20）**：

| 条件 | 转换 |
|------|------|
| CapsLock XOR Shift（组合规则以原书为准） | 在 **大小写间 toggle** |

**位运算/加减 `0x20`** — 比 **两套完整 A–Z 表** 省空间。

---

### ⑤ 键盘 LED · 物理灯同步

软件有 CapsLock 状态 → **键盘上 Caps 灯也应亮**。

| 硬件 | 操作 |
|------|------|
| **键盘控制器数据口 `0x60`** | 写 **`0xED`**（Set LEDs）+ **状态字节** |
| **位含义** | **Scroll / Num / Caps** 等对应 LED |

**闭环：** 程序内 **CapsLock 状态变** → **OUT 到键盘** → 用户 **看灯确认**。

→ [Day 7 端口 0x60 读 scancode](./day-07-FIFO与鼠标控制.md) — 同一控制器 **读写不同命令**

---

### Day 17 小结

| 问题 | 答案 |
|------|------|
| Console 怎么跑？ | 独立 **`console_task`** · 多任务并发 |
| 没事干跑谁？ | **Idle** 最低优先级 |
| 焦点怎么切？ | **Tab** · **`key_to`** · 标题栏灰/亮 |
| 键送给谁？ | **激活任务的 TASK.fifo** |
| 符号/大小写？ | **`keytable0/1` + key_shift** · **Caps ±0x20** |
| LED？ | **`0x60` · `0xED`** 设 Caps/Num/Scroll |
| 里程碑？ | **可交互 CLI 雏形** → 后续 **跑命令/程序** |

---

### 检查单

- [ ] 说清 **`console_task` vs HariMain`** 分工
- [ ] 描述 **Tab + key_to + 标题栏反馈**
- [ ] 画出 **scancode → 路由 → 任务 fifo → Console**
- [ ] 解释 **双 keytable** 与 **CapsLock/0x20**
- [ ] 知道 **0xED LED 命令** 与读 scancode 同端口不同语义

---

← [Day 16](./day-16-多任务2.md) · [08-1 导读](../README.md) · [Day 18](./day-18-dir命令.md)
