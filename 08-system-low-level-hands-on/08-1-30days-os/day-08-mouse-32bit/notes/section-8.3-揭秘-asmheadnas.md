## ③ 揭秘 asmhead.nas · 32 位模式之路

Day 3 **用过** 32 位结果；Day 8 **补全引导里 ~100 行汇编**。

#### A20GATE · 突破 1MB

| 背景 | 操作 |
|------|------|
| 早期 PC **兼容模式** 默认 **只认 ~1MB** | 开 **A20 地址线** 才能访问 **更高物理内存** |
| 经 **键盘控制器附属端口** | 写 **`0xDF`** 等序列（原书流程）→ **A20 有效** |

#### 保护模式 · `CR0` + 复制 bootpack

| 步骤 | 说明 |
|------|------|
| 设 **`CR0`** 相应位 | 进入 **32 位保护模式** |
| 汇编 **`memcpy`** | 把 **`bootpack.hrb`** 拷到 **`0x00280000`** |
| **`far-JMP`** | 跳到 **OS 本体代码段 `0x1b`** 等入口 → **开始跑 C（HariMain 链）** |

```
实模式 asmhead（BIOS、读盘、A20、GDT…）
        │
        ▼ CR0=1 · 32bit
memcpy bootpack → 0x00280000
        │
        ▼ far-JMP
HariMain / bootpack.c（C 世界）
```

**与前几 Day 贯通：**

| 机制 | 出现日 |
|------|--------|
| **GDT / IDT** | Day 5–6 初始化 |
| **PIC / ISR / FIFO** | Day 6–7 |
| **栈、IRETD** | Day 6 |
| **BOOTINFO、VRAM** | Day 4–5 |
| **IPL 读盘** | Day 3 |

Day 8 把 **「怎么进 32 位 + 大内存」** 与 **「键鼠已可用」** 收束在一起 — **底层硬件初始化拼图完成**。

→ [01-CSAPP Ch9 虚拟内存](../../../../01-CSAPP-3rd/chapter-09-virtual-memory/) · [05-LKD 启动路径](../../../../05-Linux-Kernel-Development/)

---
