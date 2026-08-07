## 11.6 综合：Tiny Web Server

> **Ch11 §11.6** · [章导读](../README.md) · 上节 [§11.5 ←](./section-11.5-Web服务器.md) · 下节 [§11.7 →](./section-11.7-小结.md)

---

流程摘要：

1. `accept` 连接
2. **读请求行** — `rio_readlineb` 解析 method / URI
3. **静态** — `stat` + `mmap` 文件 + `rio_writen` 响应头+body
4. **动态** — 调用 `serve_dynamic` 等
5. `close`

**与 HFT 类比：**

- **HTTP 解析** ≈ 任意 **文本行协议** admin API（风控面板、健康检查）
- **定长/二进制行情** 不用 HTTP — 但 **读请求头 + 路由** 模式类似
- Tiny 的 **每连接一个迭代** — 生产用 **线程池 / epoll reactor**

---

### 常见陷阱
1. **Tiny 是教学版本，每连接一个迭代** — 生产用线程池/epoll reactor，不能阻塞处理单个连接
2. **rio_readlineb 逐行读** — 适合 HTTP 文本协议；二进制行情用固定长度 read，不用逐行
3. **Tiny 的静态文件用 mmap** — 和 Ch9 mmap 联动：映射文件到 VA，直接 write 发送

### 自测题

<details>
<summary>Q1: Tiny Web Server 处理一个请求的完整流程？</summary>

1) accept 连接；2) rio_readlineb 读请求行，解析 method/URI；3) 静态：stat+mmap 文件+rio_writen 响应头+body；动态：调用 serve_dynamic；4) close 连接。

</details>

<details>
<summary>Q2: Tiny 的「每连接一个迭代」模式有什么问题？生产怎么解决？</summary>

问题：处理一个连接时阻塞，其他连接等待。生产解决：1) 线程池（每连接一个线程）；2) epoll reactor（单线程非阻塞多路复用）；3) 协程（轻量级并发）。

</details>

<details>
<summary>Q3: Tiny 用 rio_readlineb 读 HTTP 请求行，HFT 行情协议怎么读？</summary>

HTTP 是文本行协议，用 rio_readlineb 逐行读。HFT 行情是定长二进制协议，用固定长度 read（如 recv(fd, buf, sizeof(msg), 0)），不逐行读。

</details>

<details>
<summary>Q4: Tiny 的静态文件服务用 mmap，和 Ch9 学的 mmap 有什么关系？</summary>

Tiny 用 mmap 将磁盘文件映射到 VA，然后直接 write/munmap 发送。利用了 Ch9 学的：mmap 创建 VA→文件映射，首次访问 page fault 装入数据，内核页缓存自动管理。

</details>

---

← [§11.5 ←](./section-11.5-Web服务器.md) · [本章导读](../README.md) · [§11.7 →](./section-11.7-小结.md)
