# 第 7 章：套接字选项（厚版）

> [Ch 6](../Chapter06_IO_Select_Poll/study.md) → **Ch 7** → [Ch 8](../Chapter08_BasicUDPSocket/study.md)  
> 逐节：`7.x_*.md`

## 本章目标

掌握 **getsockopt/setsockopt**、**level/optname**、**监听套接字选项继承**、**SOL_SOCKET 高频项**、**TCP_NODELAY**、**fcntl 非阻塞**。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 7.1 | [7.1_Overview](./7.1_Overview.md) | 三大入口 |
| 7.2 | [7.2_Getsockopt_Setsockopt](./7.2_Getsockopt_Setsockopt.md) | API、两类选项 |
| 7.3 | [7.3_Option_Check_DefaultValue](./7.3_Option_Check_DefaultValue.md) | 探测支持 |
| 7.4 | [7.4_Socket_State_Rule](./7.4_Socket_State_Rule.md) | 继承、设置时机 |
| 7.5 | [7.5_Common_Socket_Option](./7.5_Common_Socket_Option.md) | **SOL_SOCKET 全集** |
| 7.6 | [7.6_IPv4_Socket_Option](./7.6_IPv4_Socket_Option.md) | IPPROTO_IP |
| 7.7 | [7.7_ICMPv6_Socket_Option](./7.7_ICMPv6_Socket_Option.md) | ICMP6_FILTER |
| 7.8 | [7.8_IPv6_Socket_Option](./7.8_IPv6_Socket_Option.md) | PATHMTU、V6ONLY |
| 7.9 | [7.9_TCP_Socket_Option](./7.9_TCP_Socket_Option.md) | MSS、**NODELAY** |
| 7.10 | [7.10_SCTP_Socket_Option](./7.10_SCTP_Socket_Option.md) | SCTP 选项 |
| 7.11 | [7.11_Fcntl_Control_Func](./7.11_Fcntl_Control_Func.md) | O_NONBLOCK |
| 7.12 | [7.12_Summary](./7.12_Summary.md) | 小结 |

---

## 一章速记

```text
get/set：level+optname+optval+socklen_t(值—结果)。
监听套接字选项→accept 继承；listen/connect 前设 RCVBUF。
REUSEADDR：TIME_WAIT 重启必设。
KEEPALIVE：长空闲 TCP 探测。
LINGER：0默认；linger=0→RST无TIME_WAIT；linger>0阻塞close。
TCP_NODELAY：禁 Nagle，防与延迟ACK互等。
fcntl：F_GETFL；flags|=O_NONBLOCK；F_SETFL。
```

---

## 与前面章节挂钩

| 前面问题 | 本章选项 |
|----------|----------|
| Ch2 TIME_WAIT bind 失败 | **SO_REUSEADDR** |
| Ch5 主机死不发数据 | **SO_KEEPALIVE** |
| Ch6 select 就绪条件 | **SO_RCVLOWAT/SNDLOWAT** |
| Ch6 服务器阻塞 read | **fcntl O_NONBLOCK** → Ch16 |
