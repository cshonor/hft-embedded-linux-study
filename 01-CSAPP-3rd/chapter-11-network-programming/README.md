# Ch 11 网络编程 · Network Programming

> **CSAPP 3rd** · Bryant & O'Neill · **精读 🔴**（Part III）

> 本章定位：**socket API + TCP/IP 程序员视角** — 从客户端-服务器模型到 `socket/connect/bind/listen/accept`、`getaddrinfo`、Echo 与 Tiny Web。HFT **订单/行情通道** 的 POSIX 地基；极致延迟再叠 [10-UNP](../../11-UNP-Vol1/) · [13-DPDK](../../14-DPDK-Low-Latency-Network/)。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 11.1 客户端-服务器编程模型 | [notes/section-11.1-客户端-服务器编程模型.md](./notes/section-11.1-客户端-服务器编程模型.md) |
| 11.2 网络 | [notes/section-11.2-网络.md](./notes/section-11.2-网络.md) |
| 11.3 全球IP互联网 | [notes/section-11.3-全球IP互联网.md](./notes/section-11.3-全球IP互联网.md) |
| 11.4 套接字接口 | [notes/section-11.4-套接字接口.md](./notes/section-11.4-套接字接口.md) |
| 11.5 Web服务器 | [notes/section-11.5-Web服务器.md](./notes/section-11.5-Web服务器.md) |
| 11.6 综合TinyWebServer | [notes/section-11.6-综合TinyWebServer.md](./notes/section-11.6-综合TinyWebServer.md) |
| 11.7 小结 | [notes/section-11.7-小结.md](./notes/section-11.7-小结.md) |


---

## 大白话 · 本章一条线

> **网络 = 另一台机器上的进程；socket 是内核里的「电话插口」，TCP 是可靠字节流。**

```
getaddrinfo → socket → bind/listen/accept（服务端）
                    → connect（客户端）
read/write 收发字节（可能 short count）
```

**HFT 三件事：**

1. **字节序与定长协议** — 线上 big-endian；别用 `strlen` 解析二进制
2. **TCP 客户端/服务端模型** — 行情 feed + 订单网关；多连接用 **非阻塞 + epoll**（UNP 延伸）
3. **本章是阻塞 Echo** — 教学用；生产加 **NODELAY、非阻塞、线程/ reactor**（→ Ch12）

---

## 本章 Checklist

- [ ] 画出客户端-服务器模型；TCP vs UDP 适用场景
- [ ] 会写 `sockaddr_in` / `getaddrinfo` 协议无关解析
- [ ] 能手写 **阻塞式 Echo 客户端/服务器** 流程
- [ ] 知道 `listen` backlog、`accept` 返回新 fd
- [ ] 读懂 HTTP 请求行；Tiny 如何 `mmap` 静态文件
- [ ] 说出 HFT 在 socket 层常加的选项：`TCP_NODELAY`、非阻塞

---

## HFT 精读捷径

```
11.1–11.4 套接字 API — 必读
11.3 IP/端口/连接 — 与 10-TCP-IP 交叉
11.5–11.6 Tiny/HTTP — 选读（理解 request/response 解析）
延伸：08-UNP epoll · 09 内核网络 · 10 DPDK 旁路 · 03-SysPerf Ch10
```

---

## 相关章节

- 上一章：[../chapter-10-system-io/](../chapter-10-system-io/)
- 下一章：[../chapter-12-concurrent-programming/](../chapter-12-concurrent-programming/)
- 字节序：[../chapter-02-representing-information/](../chapter-02-representing-information/)
- 协议理论：[12-TCP-IP-Illustrated-Vol1](../../12-TCP-IP-Illustrated-Vol1/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
