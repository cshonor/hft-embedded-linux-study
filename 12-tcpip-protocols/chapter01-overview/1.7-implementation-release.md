# 1.7 实现和软件分发

> 章级导读：[../study.md](../study.md)

## 本节核心目标

建立 TCP/IP **实现史** 脉络，理解 **BSD 网络栈** 对现代操作系统的影响。

---

## Berkeley 软件分发 (BSD)

- BSD 系统附带完整、可研究的 **TCP/IP 实现**（如 4.2BSD、4.3BSD、**4.4BSD-Lite**）。
- 许多 API 语义、数据结构、ioctl 行为源自 BSD，成为事实标准。

---

## 对现代系统的影响

| 系统族 | 关系 |
|--------|------|
| Linux | 网络栈与 Socket 语义大量借鉴 BSD 传统 |
| macOS / iOS | 源自 BSD 系 |
| Windows | Winsock 抽象与 BSD Socket **概念对齐**（API 细节不同） |

---

## 学习意义

- 读 Stevens 示例代码时，**errno、非阻塞、SIGIO** 等常与 BSD 起源相关。
- 改内核/驱动 vs 改应用：分层在 1.2 节，实现边界在本节与后续各协议章。
