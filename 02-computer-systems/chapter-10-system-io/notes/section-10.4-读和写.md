## 10.4 读和写

> **Ch10 §10.4** · [章导读](../README.md) · 上节 [§10.3 ←](./section-10.3-打开和关闭.md) · 下节 [§10.5 →](./section-10.5-Rio包.md)

---

← [本章导读](../README.md)

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. `read()` 和 `write()` 为什么可能返回比请求少的字节数？</summary>

`read` 可能返回 < n：1. 遇到 EOF
2. 从终端读遇到换行
3. 网络socket 可用数据不足
4. 被信号中断(EINTR)

`write` 可能返回 < n：1. 磁盘满
2. 管道/socket 缓冲区满
3. 被信号中断

**必须循环**直到读满/写满 n 字节——这就是 CSAPP Rio 包的作用。HFT 注意：每次 `read`/`write` 都是系统调用（~1μs 开销），应批量处理减少调用次数。

</details>


---

← [§10.3 ←](./section-10.3-打开和关闭.md) · [本章导读](../README.md) · [§10.5 →](./section-10.5-Rio包.md)
