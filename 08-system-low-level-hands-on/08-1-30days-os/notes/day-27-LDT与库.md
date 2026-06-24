## Day 27 · LDT 与库

> **原书第二十七章** · **安全 + 工程化** — 修 **ncst** 关闭、**LDT** 防 **crack7**、API **.obj 拆分**、**`apilib.lib`**、目录与 **`app_make.txt`**。  
> ← [Day 26](./day-26-为窗口移动提速.md) · [08-1 导读](../README.md) · → Day 28（待补）

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① ncst 修复** | **Shift+F1 / ×** 可关 | 先 **藏窗** 再后台释放 |
| **② LDT** | 每 task **局部段表** | **crack7** 跨 app 读写 **阻断** |
| **③ 拆分 API** | 一函数一 **.obj** | **hello3** 不再 **520B 虚胖** |
| **④ Library** | **`apilib.lib`** | Librarian 打包 |
| **⑤ 工程重构** | **haribote/apilib/apps** · **`app_make.txt`** | **`make run_full`** |

---

### ① 修复 ncst 关闭 · 体验优化

Day 26 **`ncst`** 启 GUI — **Shift+F1 / × 无反应** → app **关不掉**。

| 修复 | 说明 |
|------|------|
| **梳理关窗/杀 task 路径** | **ncst 无 Console** 任务也进 **统一终止逻辑** |
| **先 hide 再 free** | 点击 × → **立刻从画面消失** → **后台** 释 sheet/MEMMAN/timer |

**感知延迟 ↓** — 与 Day 24 **timer 清理**、Day 23 **sheet→task** 同一 **生命周期链**。

→ [Day 26 ncst](./day-26-为窗口移动提速.md)

---

### ② LDT · 局部段记录表

#### 缺口 · crack7.hrb

Day 21 **GDT** 隔离 **OS vs 单 app**；Day 25 **动态段号** 隔离 **并发 app 数据**。

**crack7** 证明：**恶意 app 仍可读写其他 app 的段** — GDT 条目 **全局可见**。

#### LDT vs GDT

| | **GDT** | **LDT** |
|--|---------|---------|
| 范围 | **全局** — OS + 所有任务可见描述符 | **每任务私有** |
| 用途 | 内核段、TSS、公共描述符 | **该 app 专属 CS/DS** |
| CPU | **LGDT** | 任务切换时 **LLDT / TSS 中 LDT** |

**每个 TASK 自己的 LDT** 注册 **本 app 代码/数据段** → CPU **无法加载别 task 的 LDT 项** → **硬件级 app↔app 隔离**。

```
app A 的 LDT:  仅 A 的 CS/DS
app B 的 LDT:  仅 B 的 CS/DS
A 想 MOV DS, B的段 → #GP
```

**多任务安全闭环** — 配合 Day 21 **0x0d**、Day 25 **sel 映射**。

→ [Day 5/6 GDT](./day-05-结构体文字显示与GDT-IDT.md) · [05-LKD 内存保护](../../05-Linux-Kernel-Development/)

---

### ③ 拆分 API 目标文件 · 应用程序瘦身

#### 问题

**`a_nask.nas`** 整包链进 **每个 .hrb** → **`hello3` 极简** 也 **~520B**（未用 API 汇编仍在）。

#### 解法

**每个 API 函数 → 独立 `.obj`** — 链接器 **只拉被 CALL 的符号**。

| 之前 | 之后 |
|------|------|
| 全量 `a_nask` | **按需 link** |
| hello3 **520B** | **显著减小**（原书对比） |

**死代码消除** 在 **链接期** — 与 **`-ffunction-sections --gc-sections`** 思想一致。

→ [Day 20 `_api_*`](./day-20-API.md)

---

### ④ 库（Library）· apilib.lib

拆分后 **成百 `.obj`** — Makefile **写不完**。

| 工具 | 作用 |
|------|------|
| **Librarian（库管理器）** | 把 API **.obj 归档** 成 **`apilib.lib`** |
| app 链接 | **`apilib.lib` + 自有 .obj** — 仍 **按需取成员**（静态库符号解析） |

**一次打包、处处复用** — C 运行时 **`libc.a`** 同源。

---

### ⑤ 整理 Make · 目录重构

#### 目录

| 目录 | 内容 |
|------|------|
| **`haribote/`** | OS 内核 |
| **`apilib/`** | API 库源码 → **`apilib.lib`** |
| **各 app 子目录** | `hello3/`、`color2/` … |

#### `app_make.txt`

**`include` 共通规则** — 每个 app 的 **Makefile 仅数行**。

#### 全局目标

| 命令（示意） | 用途 |
|--------------|------|
| **`make run_full`** | 编全盘 + 跑 |
| **`make install_full`** | 写入软盘映像 |

**从手工作坊 → 可维护工程** — HFT 里 **monorepo + 共享 static lib + 顶层 CMake** 同构。

→ [Day 2 Makefile](./day-02-汇编语言与Makefile入门.md) · [Day 6 模块拆分](./day-06-分割编译与中断处理.md)

---

### Day 27 小结

| 问题 | 答案 |
|------|------|
| ncst 关不掉？ | **统一 kill 路径** · **先 hide** |
| crack7 说明啥？ | **GDT 不够 app 互防** |
| LDT 干嘛？ | **每 task 私有段表** |
| hello3 为何大？ | **整包 API asm** → **按 .obj 链接** |
| 碎 obj 怎么管？ | **`apilib.lib`** |
| 工程？ | **haribote/apilib/apps** + **`app_make.txt`** |

---

### 检查单

- [ ] 对比 **GDT（OS/全局）vs LDT（单 app）**
- [ ] 说清 **静态库按需链接** 与体积关系
- [ ] 知道 **`apilib.lib`** 在构建链位置
- [ ] 描述 **ncst 关闭 UX（先藏后释）**
- [ ] 串 Day 21→25→27 **安全演进**

---

← [Day 26](./day-26-为窗口移动提速.md) · [08-1 导读](../README.md) · Day 28（待补）
