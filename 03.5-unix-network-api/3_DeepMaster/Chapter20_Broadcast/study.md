# 第 20 章：广播（厚版）

> [Ch 17](../Chapter17_Ioctl_Operate/study.md) · **Ch 20** · [Ch 21](../Chapter21_Multicast/)（待笔记）  
> 逐节：`20.x_*.md`

> **说明**：上传资料截至第 8 章；第 20 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握 IPv4 **广播地址**、相对单播的**链路层代价**、**SO_BROADCAST**、广播 **dg_cli**、以及 **alarm 竞态** 与 **pselect** 解法。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 20.1 | [20.1_Overview](./20.1_Overview.md) | UDP only；IPv6→多播 |
| 20.2 | [20.2_Broadcast_Address](./20.2_Broadcast_Address.md) | 四类广播地址 |
| 20.3 | [20.3_Unicast_Broadcast_Compare](./20.3_Unicast_Broadcast_Compare.md) | 接收路径与 CPU 代价 |
| 20.4 | [20.4_Broadcast_Dg_Cli](./20.4_Broadcast_Dg_Cli.md) | SO_BROADCAST、MTU |
| 20.5 | [20.5_Race_Condition_Problem](./20.5_Race_Condition_Problem.md) | pselect |
| 20.6 | [20.6_Summary](./20.6_Summary.md) | 全章收束 |

---

## 一章速记

```text
仅 UDP；IPv6 无广播
地址：255.255.255.255 / 子网 .255；路由器一般不转发
sendto 广播前 setsockopt(SO_BROADCAST)，否则 EACCES
载荷 < MTU，通常不分片
一发多收：勿 alarm+recvfrom 裸用；用 pselect 或 select 超时
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 7.5 | SO_BROADCAST |
| Ch 8 | dg_cli、recvfrom 超时 |
| Ch 6.9 | pselect |
| Ch 17.7 | SIOCGIFBRDADDR |
| Ch 21 | 多播替代广播 |
