# 第8章 存储器系统

> **定位：** Ch7 CPU 假定「访存一拍好」——本章拆穿：CPU 与 DRAM 速度鸿沟 → **层次结构**（Cache + VM）换大容量、高速度、低成本。  
> **对照：** ↔ CSAPP Ch6（Cache）/ Ch9（VM）· [hft_x86_timing](../cross_ref/hft_x86_timing.md)

**层次直觉：**

```
CPU ↔ L1/L2 SRAM Cache ↔ DRAM 主存 ↔ 磁盘/SSD（VM 后备）
     快·贵·小              中               慢·便宜·大
```

**对你两条线：**

| 路线 | 作用 |
|------|------|
| **嵌入式 Linux** | **Cache 一致性 / DMA**；**MMU、ioremap、用户指针、缺页** |
| **HFT** | Cache/TLB miss 延迟；伪共享；大页 |

**本路线：** **8.3 + 8.4 精读**（行为与驱动 API）；细节公式可回 CSAPP Ch6/Ch9。

| 小节 | 档位 | 笔记 |
|------|------|------|
| 8.1 引言 | 浅读 | [8.1_引言.md](./8.1_引言.md) |
| 8.2 性能分析 | 中读 · AMAT | [8.2_存储器系统性能分析.md](./8.2_存储器系统性能分析.md) |
| 8.3 高速缓存 | **精读** · DMA | [8.3_高速缓存.md](./8.3_高速缓存.md) |
| 8.4 虚拟存储器 | **精读** · ioremap | [8.4_虚拟存储器.md](./8.4_虚拟存储器.md) |
| 8.5 总结 | 浅读 | [8.5_总结.md](./8.5_总结.md) |

← [书仓总 README](../README.md) · 上章 [ch07](../ch07_microarchitecture/README.md) · 在线 [ch09 I/O](../ch09_io_online/README.md) · [cross_ref](../cross_ref/csapp_ch4_link.md)
