## 11.5 Web 服务器

> **Ch11 §11.5** · [章导读](../README.md) · 上节 [§11.4 ←](./section-11.4-套接字接口.md) · 下节 [§11.6 →](./section-11.6-综合TinyWebServer.md)

---

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

---

### 常见陷阱
1. **HTTP 是无状态协议** — 每个请求独立，服务器不记得之前的请求；keep-alive 复用 TCP 连接但不保持应用状态
2. **静态内容读磁盘，动态内容执行程序** — CGI 思想：URI 映射到可执行程序，程序输出即为 HTTP body
3. **HFT 不用 HTTP 传行情** — HTTP 头部开销大、文本解析慢；但 admin API（风控面板、健康检查）可用 HTTP

### 自测题

<details>
<summary>Q1: HTTP 请求和响应的基本格式是什么？</summary>

请求：方法(GET/POST) + URI + 版本 + 头部 + 空行(\r\n) + 可选body。响应：版本 + 状态码 + 头部 + 空行(\r\n) + body。每行以 \r\n 结尾。

</details>

<details>
<summary>Q2: HTTP 的无状态是什么意思？keep-alive 改变了这一点吗？</summary>

无状态 = 每个请求独立处理，服务器不记录之前请求的状态。keep-alive 复用 TCP 连接（减少握手开销），但应用层仍无状态。状态通过 Cookie/Session 在应用层维护。

</details>

<details>
<summary>Q3: 静态内容和动态内容的服务方式有何不同？</summary>

静态：直接读磁盘文件返回（HTML/图片）。动态：解析 URI，执行对应程序（CGI），程序输出作为 HTTP body 返回。Tiny 用 fork+execve 或函数指针表。

</details>

<details>
<summary>Q4: HFT 为什么不用 HTTP 传输行情？什么场景用 HTTP？</summary>

HTTP 头部开销大（百字节文本）、解析慢（文本协议）、无二进制支持。行情用 UDP multicast 或 TCP 自定义二进制协议。HTTP 用于 admin API（风控面板、健康检查、配置管理）。

</details>

---

← [§11.4 ←](./section-11.4-套接字接口.md) · [本章导读](../README.md) · [§11.6 →](./section-11.6-综合TinyWebServer.md)
