# 24.6 小结

> [study.md](../study.md) · [Ch 25 信号驱动 I/O](../Chapter25_SignalDriveIO/study.md)

---

## 章节核心提炼

### 1. 逻辑机制 > 物理通道

TCP OOB = 字节流上的 **URG + 紧急指针**，非第二根线缆 → **易覆盖、语义混乱**。

### 2. 双模式读取

| 模式 | API |
|------|-----|
| 分离（默认） | `recv` + **MSG_OOB** |
| 线内 | **SO_OOBINLINE** + **sockatmark** |

### 3. 现代地位

除 **Telnet 中断（Ctrl+C）**、**OOB 心搏** 外，HTTP/gRPC 等多在**应用层**用独立控制帧/Stream，**几乎不用 TCP 层 OOB**。

---

## 与相邻章

| 章 | 关联 |
|----|------|
| Ch 14 | MSG_OOB |
| Ch 17 | SIOCATMARK → sockatmark |
| Ch 25 | SIGIO / 信号驱动（另一异步路径） |

---

## 个人学习总结

（待填）
