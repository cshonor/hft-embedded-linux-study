## §5.7 内存注意事项 — 链接与段放置

> **Ch 5 · 加载、存储与寻址** · [章导读](../README.md)

---

### 汇编器/链接器在幕后做什么

源码里的 **`SPACE` 留栈**、**`AREA` 代码**、**`DCD` 常量** 最终必须落到芯片 **真实 Flash / SRAM 地址** — 由 **链接器 + 配置文件** 决定。

```
.s / .c  →  .o（段 + 重定位）
              ↓
链接器 + Scatter / Linker Command File
              ↓
.axf / .elf — 各段 VMA/LMA（Flash 里存什么、RAM 里跑什么）
```

| 概念 | 说明 |
|------|------|
| **Scatter-Loading (Keil)** | 分散加载 — 指定 **RO → Flash**、**RW/ZI → RAM** |
| **Linker Command File (CCS)** | TI 的 `.cmd` — 同样语义 |
| **GNU** | **链接脚本** `.ld`（→ [Ch4 §4.4](../../chapter-04-assembler-rules-directives/notes/section-4-4-directives.md) · [奔跑吧 Ch9](../../../arm64-programming-practice/chapter-09-linker-scripts/)） |

---

### 典型放置

| 段 | 内容 | 物理位置 |
|----|------|----------|
| **RO (Code + RO Data)** | 指令、只读常量 | **Flash / ROM** |
| **RW (Initialized Data)** | 有初值全局变量 | **RAM**（启动时从 Flash 拷贝） |
| **ZI (BSS)** | 零初始化变量、`SPACE` 栈 | **RAM** |
| **Stack / Heap** | `SPACE` 或链接脚本 `_stack_top` | **RAM 高端** |

**启动代码职责：** 拷贝 `.data`、清零 `.bss`、设 **SP** — 然后才 `main`（→ U-Boot/`head.S` 同类）。

---

### 与 Ch4/Ch5 指令的衔接

| 源码写法 | 链接后 |
|----------|--------|
| `my_var SPACE 256` | RAM 中占 256 字节 — **`LDR/STR` 用 &my_var** |
| `code AREA CODE` | Flash 中 — CPU 取指 |
| `DCD 0x40001000` | 常数池 — **`LDR =` 加载到寄存器再 STR 到外设** |

**错链接脚本后果：** 变量写到 **不存在的 RAM**、栈与堆重叠 — 运行随机 fault。

---

### 嵌入式 Linux 尺度

| MCU | Linux |
|-----|-------|
| Scatter / `.ld` 定 SRAM | **设备树** 报内存 `@0x80000000, size` |
| 单镜像 | **U-Boot + kernel + dtb** 各段加载地址 |
| [20 构建](../../../../20-UBoot-Kernel-Build/) | Yocto/Buildroot 打包同一逻辑 |

---

### 可复述要点

1. **Load/Store 的地址** 来自链接器分配的符号 — 不是魔法数字（除非 MMIO `EQU`）。  
2. **Scatter/链接脚本** = Flash 放代码、RAM 放数据/栈。  
3. 启动代码在 **第一条 LDR 业务数据前** 必须设好 SP 与内存初始化。
