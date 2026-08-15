## 2. 块设备处理的层次结构

> 一次 **read/write** 穿越的内核组件

---

### 一、自上而下五层

```
进程 syscall（read / write）
    ↓
① VFS              — 系统调用、file 对象
    ↓
② Mapping Layer    — 文件偏移 → 磁盘 **逻辑块号**
    ↓
③ Generic Block Layer — 统一块 I/O 抽象；构造 **bio**
    ↓
④ I/O Scheduler    — 合并、排序 **request**
    ↓
⑤ Block Device Driver — DMA / 控制器命令 → 磁盘
```

每层 **隐藏下层细节**，上层只面对统一接口。

---

### 二、为何需要这么多层

| 问题 | 哪层解决 |
|------|----------|
| 文件语义 vs 裸扇区 | Mapping Layer + 具体 FS |
| 不同磁盘/控制器差异 | Generic Block Layer |
| 磁头频繁寻道 | I/O Scheduler |
| 硬件命令与 IRQ | Block Driver |

→ VFS：[Ch 12](../chapter-12-VFS/) · 页缓存减少穿透：[Ch 15](../chapter-15-page-cache/)

---

### 三、与字符设备对比

| | 字符设备 (Ch 13) | 块设备 (本章) |
|--|------------------|---------------|
| 单位 | 字节流 | **固定块**、可随机寻址 |
| 缓存 | 通常无块层缓存 | **页缓存** + 请求队列 |
| 典型 | 串口、键盘 | 硬盘、SSD（仍走块层） |

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 扇区/块/段](./section-3-扇区块与段.md)
