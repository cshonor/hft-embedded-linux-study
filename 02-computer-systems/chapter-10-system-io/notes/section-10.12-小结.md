## 10.12 小结（原书）

> **Ch10 §10.12** · [章导读](../README.md) · 上节 [§10.11 ←](./section-10.11-综合该用哪些I-O？.md) · 下节 —

---

← [本章导读](../README.md)

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. Unix I/O 模型对 HFT 的核心启示是什么？</summary>

1. **一切皆文件 + fd**：统一接口简化编程，但 fd 操作有系统调用开销
2. **read/write 不保证满**：必须循环或用 Rio
3. **三级表结构**：理解 fork/dup 共享 offset 的原理
4. **非阻塞 I/O + epoll**：HFT 网络的标准模式
5. **缓冲是双刃剑**：减少系统调用但增加延迟
6. **极限性能绕过内核**：DPDK/onload 用用户态网卡驱动，不经 fd/read/write

CSAPP Ch10 是理解网络编程(Ch11)和并发(Ch12)的基础——epoll 本质上就是管理 fd 的 I/O 多路复用。

</details>


---

← [§10.11 ←](./section-10.11-综合该用哪些I-O？.md) · [本章导读](../README.md) · —
