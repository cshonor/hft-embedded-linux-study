## Day 20 · API

> **原书第二十章** · **内核 ↔ 应用桥梁** — **far-CALL/RETF**、**INT 0x40**、**`cmd_app`**、**PUSHAD/POPAD**、**EDX 功能号** 路由。  
> ← [Day 19](./day-19-应用程序.md) · [08-1 导读](../README.md) · → [Day 21](./day-21-保护操作系统.md)

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 首个 API** | 硬编码 **CALL** | **far-CALL + RETF** 回 Console |
| **② INT 0x40** | API 挂 **IDT** | **地址无关** · 更小指令 |
| **③ cmd_app** | 未知命令 → **`.hrb`** | 任意文件名运行 |
| **④ 寄存器保护** | **PUSHAD/POPAD** | API 不破坏 app 循环 |
| **⑤ EDX 路由** | **功能号 1/2/3…** | 一中断 **多 API** |

---

### ① 第一个 API · 优雅结束程序

#### OS 的价值

应用 **不应** 自己画每个像素 — 应 **调用 OS 提供的服务**。

#### 初版：绝对地址 + CALL

| 做法 | 问题 |
|------|------|
| app 里 **写死 OS 函数物理/线性地址** | **CALL** 跳转 |

能 **在 Console 显示一个字符** — 但 **脆弱**（Day ② 详述）。

#### 程序结束 · 不再 HLT 卡死

Day 19 **`hlt.hrb`** 结束 → **HLT 死循环** → **Shell 再也收不到输入**。

| 机制 | 方向 |
|------|------|
| OS 用 **`far-CALL`** 启动 app | OS → app |
| app 结尾 **`RETF`（远返回）** | app → **回到 OS Console** |

```
Console → far-CALL app段:入口
              app 运行 …
              RETF → 继续 Shell 读命令
```

→ [Day 19 farjmp](./day-19-应用程序.md) · [Day 6 IRETD/栈](./day-06-分割编译与中断处理.md)

---

### ② INT 0x40 · 稳定 API 入口

#### 硬编码地址的致命伤

**OS 任一改动** → 函数 **重定位** → **旧 .hrb 全崩**。

#### 中断做 syscall 门

| 事实 | 利用 |
|------|------|
| IDT **0~255**，键鼠等只用少数 | **空闲向量**，如 **`0x40`** |
| CPU **`INT n`** | 固定 **2 字节**，不 embed 地址 |

**注册：** IDT[0x40] → **API 汇编入口**（再 **PUSHAD / 分发 / POPAD / IRET** 或 **IRET/RETF** 链，以原书为准）

```nasm
; app 侧
MOV EDX, 1          ; 功能号（见 §⑤）
INT 0x40            ; 呼叫 OS — 与 OS 版本解耦
```

| 对比 | far-CALL 硬地址 | **INT 0x40** |
|------|-----------------|--------------|
| OS 升级 | app 需重编 | **app 不变**（接口稳定） |
| 代码体积 | CALL 远指针更长 | **INT 更短** |

**现代对照：** x86 **`syscall`/`sysenter`**、Linux **号 + 寄存器** — 本课是 **INT 0x40 教学版**。

→ [Day 5/6 IDT](./day-05-结构体文字显示与GDT-IDT.md) · [07-TLPI syscall](../../07-The-Linux-Programming-Interface/)

---

### ③ 自由运行任意应用程序 · `cmd_app`

之前：**只能敲固定命令**（如 **`hlt`**）。

**改 Console 解析：**

```
strcmp 内置命令 (mem/cls/dir/type…) 
    → 命中则执行
else
    cmd_app(输入行) → FAT 找 **同名.hrb** → load + run
```

**用户输入 `hello` → 运行 `hello.hrb`** — **Shell 即程序加载器**。

→ [Day 18/19 FAT + loadfile](./day-18-dir命令.md)

---

### ④ 寄存器保护 · PUSHAD / POPAD

#### Bug

app 用 **ECX 循环** 调 API 打印一串字符 → **只出第一个**。

#### 根因

OS 内 **C 函数** 编译后 **随意用 ECX 等** — API 返回后 **app 寄存器被踩**。

#### 修复

**API 汇编入口：**

```nasm
api_handler:
    PUSHAD          ; 保存 EAX..EDI 等
    ; … 调 C 实现 …
    POPAD           ; 恢复
    IRETD / RETF    ; 返回 app
```

**ABI 规则：** **内核入口保存 caller-saved 全集** — 直到 app 自己约定 calling convention。

**HFT：** syscall **内核不能破坏用户寄存器**（除返回值约定）；**WRONG** → 极难查的 **heisenbug**。

→ [01-CSAPP Ch3 调用约定](../../01-CSAPP-3rd/chapter-03-machine-level-programs/)

---

### ⑤ 扩展 API · EDX 功能号路由

**一个 INT 0x40** — 多种服务 → 学 **BIOS：用寄存器带「子功能号」**。

| **EDX** | API（原书） |
|---------|-------------|
| **1** | 显示 **单字符** |
| **2** | 显示 **以 0x00 结尾** 的字符串 |
| **3** | 显示 **指定长度** 字符串 |

**汇编入口伪逻辑：**

```
INT 0x40 进入
    PUSHAD
    CMP EDX, 1 → putchar
    CMP EDX, 2 → puts0
    CMP EDX, 3 → putsn
    …
    POPAD
    返回 app
```

**EDX 32 位** → 理论上 **2³² 种功能扩展** — 后续 **开窗、读键、alloc** 都挂同一 **0x40** 即可。

---

### Day 20 小结

| 问题 | 答案 |
|------|------|
| 首个 API 干嘛？ | **Console 出字符** |
| 如何回到 Shell？ | OS **far-CALL** · app **RETF** |
| 为何 INT？ | **OS 改版 app 不碎** · 指令更短 |
| 向量号？ | **0x40**（IDT 空闲槽） |
| 任意程序？ | **`cmd_app`** → **`xxx.hrb`** |
| 只显示一字？ | C 改寄存器 → **PUSHAD/POPAD** |
| 多 API？ | **`EDX` = 功能号** 分发 |

**里程碑：** **可扩展、稳定的用户态接口** — 内核与 **.hrb 生态** 正式分层。

---

### 检查单

- [ ] 对比 **硬编码 CALL vs INT 0x40**
- [ ] 说清 **far-CALL / RETF** 与 Day 19 **farjmp** 差别
- [ ] 描述 **`cmd_app`** 与内置命令分支
- [ ] 解释 **PUSHAD/POPAD** 与 ECX bug
- [ ] 列举 **EDX=1/2/3** 并理解 BIOS 式路由

---

← [Day 19](./day-19-应用程序.md) · [08-1 导读](../README.md) · [Day 21](./day-21-保护操作系统.md)
