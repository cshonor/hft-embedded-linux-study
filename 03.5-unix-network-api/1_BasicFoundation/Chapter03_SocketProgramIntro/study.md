# 第 3 章：套接字编程简介（厚版）

> [Ch 2](../Chapter02_TCP_UDP_SCTP/study.md) → **Ch 3** → [Ch 4](../Chapter04_BasicTCPSocket/study.md)

逐节见 `3.x_*.md`（3.2、3.3、3.9 为最厚小节）。

## 小节索引

3.1 概述 · 3.2 地址结构 · 3.3 值—结果 · 3.4 字节序 · 3.5 bzero · 3.6 inet_addr 系 · 3.7 pton/ntop · 3.8 sock_ntop · 3.9 readn/writen · 3.10 小结

## 速记

```text
sockaddr_in 网络序；强转 sockaddr。
accept 长度 socklen_t* 值—结果。
htons；inet_pton；勿 inet_ntoa 静态缓冲。
TCP 用 readn/writen；EINTR continue。
```
