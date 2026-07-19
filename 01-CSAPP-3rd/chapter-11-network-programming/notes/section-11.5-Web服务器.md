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

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§11.4 ←](./section-11.4-套接字接口.md) · [本章导读](../README.md) · [§11.6 →](./section-11.6-综合TinyWebServer.md)
