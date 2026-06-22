## 11.5–11.7 Web 服务器与 Tiny

### 11.5 Web 服务器

#### 11.5.1 Web 基础

- **Web 客户端（浏览器）** ↔ **Web 服务器** — 仍属 C/S，应用协议 **HTTP**

#### 11.5.2 Web 内容

- **静态** — 磁盘文件（HTML、图片）
- **动态** — 服务器运行程序生成（CGI 思想）

#### 11.5.3 HTTP 事务

典型请求：

```http
GET /index.html HTTP/1.1
Host: www.example.com
\r\n
```

响应：

```http
HTTP/1.1 200 OK
Content-Length: 1234
\r\n
<body>
```

- **无状态** — 每请求独立；连接可 **keep-alive** 复用 TCP

#### 11.5.4 服务动态内容

- 解析 URI → 执行对应处理函数 → 生成 body
- Tiny 用 **fork + execve** 或函数指针表（教学）

### 11.6 综合：Tiny Web Server

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

### 11.7 小结（原书）

- 网络 = I/O + 协议 + C/S
- **Socket API** 是 Linux 网络应用基石
- **getaddrinfo** 写可移植客户端
- Web/Tiny 展示 **应用层协议** 如何叠在 TCP 上

→ [Ch 12 并发服务器](../../chapter-12-concurrent-programming/) · [09 内核网络栈](../../../09-Linux-Kernel-Networking/)

---

← [本章导读](../README.md)
