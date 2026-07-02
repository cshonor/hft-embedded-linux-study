## ①.3 INT 0x13 与 ipl 代码拆解

> **别和「VGA 模式 0x13」搞混：** 本节 **`INT 0x13`** 是 **磁盘中断**（读软盘），不是 320×200 图形模式。图形模式是 **`INT 0x10` + `AL=0x13`** → [§3.2.1](../section-3.2.1-VGA模式0x13详解.md) · [README 易混点表](../README.md#易混点int-0x13-和模式-0x13-不是一回事)。

IPL 在 **16 位实模式** 下运行，读 bootpack 全靠 **`INT 0x13`**。本节对照 [ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm) 看 API 与三大逻辑块。

---

### 核心 API：BIOS `INT 0x13`

#### 读扇区 · `AH = 0x02`（最常用）

| 寄存器 | 含义 | 本书典型值 |
|--------|------|------------|
| **AH** | 子功能 | **`0x02`** = 读扇区 |
| **AL** | 连续读取扇区数 | **`1`**（逐扇区读，便于重试） |
| **CH** | 柱面号 C | **0~9** |
| **CL** | 扇区号 S | **1~18**（从 **2** 起读 OS） |
| **DH** | 磁头号 H | **0 / 1** |
| **DL** | 驱动器号 | **`0x00`** = A 软驱 |
| **ES:BX** | 写入内存地址 | **ES=0x0820, BX=0** → 物理地址 **0x8200** |

#### 错误判定

| 结果 | 标志 | 汇编 |
|------|------|------|
| 成功 | **CF = 0** | `JNC next` |
| 失败 | **CF = 1** | 进入重试或 `error` |

软盘机械不稳定，标准容错：**单扇区最多重试 5 次**；仍失败则打印 **load error** 并 **`HLT` 停机**。

#### 驱动器复位 · `AH = 0x00`

读取出错后调用，重置磁头、重新寻道，再 **`JMP retry`**。

> 这里的 **`AH = 0x00`** 是 BIOS **命令号**，与磁盘镜像里的 **字节 `0x00` 占位** 无关 — 见 [§3.1.1](./section-3.1.1-IPL-bootpack与镜像布局.md)。

---

### 三大核心逻辑（[ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm)）

#### 1. 失败重试 5 次

```nasm
readloop:
    MOV     SI, 0           ; 本扇区失败计数
retry:
    MOV     AH, 0x02
    MOV     AL, 1
    MOV     BX, 0
    MOV     DL, 0x00
    INT     0x13
    JNC     next            ; 成功 → 下一扇区
    ADD     SI, 1
    CMP     SI, 5
    JAE     error           ; ≥5 次 → 报错停机
    MOV     AH, 0x00        ; 复位软驱
    MOV     DL, 0x00
    INT     0x13
    JMP     retry
```

#### 2. 循环遍历 10 柱面

每读完 **512 B**，**ES 段 +0x20**（512÷16=0x20；x86 没有 `ADD ES, imm`，只好改 AX 再写回 ES）：

```nasm
next:
    MOV     AX, ES
    ADD     AX, 0x0020
    MOV     ES, AX          ; 内存指针下移 512 B
    ADD     CL, 1           ; 扇区 +1
    CMP     CL, 18
    JBE     readloop        ; 本磁道未读完
    MOV     CL, 1           ; 换磁头，扇区归 1
    ADD     DH, 1
    CMP     DH, 2
    JB      readloop        ; H0→H1
    MOV     DH, 0           ; 换柱面
    ADD     CH, 1
    CMP     CH, CYLS        ; CYLS EQU 10
    JB      readloop
```

CHS 寻址顺序见 [§3.1.2](./section-3.1.2-软盘CHS结构与读盘范围.md)。

#### 3. 启动框架 + FAT12 引导参数块（BPB）

IPL 开头除 **`JMP entry`** 外，还要填 **FAT12 磁盘头**（`"HARIBOTE"`、每扇区 512、18 扇区/磁道等），与 **1.44 MB 软盘镜像** 格式一致：

```nasm
CYLS    EQU     10
ORG     0x7C00
JMP     entry
DB      0x90
DB      "HARIBOTE"
; … BPB 字段 …
RESB    18

entry:
    MOV     AX, 0
    MOV     SS, AX
    MOV     SP, 0x7C00
    MOV     DS, AX
    MOV     AX, 0x0820
    MOV     ES, AX
    MOV     CH, 0
    MOV     DH, 0
    MOV     CL, 2           ; S1 是 IPL 自身，从 S2 读 OS
    ; → readloop / retry / next …
```

读完后打印 **`load done`**，**`JMP 0x8200`** 交给 bootpack。

完整可编译源码：[ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm) · 构建：[README.md](../code/sec-3.1-ipl-int13-disk-load/README.md)。

---

### 关键考点（代码向）

1. **读盘 API**：**`INT 0x13`, `AH=0x02`**；用 **CF** 判错。  
2. **可靠性**：单扇区 **最多 5 次重试**；失败前 **复位驱动器（`AH=0x00`）**。  
3. **加载范围**：**C0~C9** 共 10 柱面 ≈ **180 KB** → 内存 **`0x8200` 起**。  
4. **内存步进**：每扇区 **ES += 0x20**（512 字节段偏移）。  
5. **全部是 16 位实模式汇编** — 为何不能一直停在这个模式，见 [§3.1.4](./section-3.1.4-实模式读盘与保护模式切换.md)。

---

← [§3.1.2 软盘 CHS](./section-3.1.2-软盘CHS结构与读盘范围.md) · [§3.1.4 实模式 vs 保护模式 →](./section-3.1.4-实模式读盘与保护模式切换.md)
