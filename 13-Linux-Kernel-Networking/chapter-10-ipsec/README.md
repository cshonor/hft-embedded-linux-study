# Ch 10 IPsec

> **Linux Kernel Networking** · Rami Rosen · **跳过 ⚪**

> **IPsec** — AH/ESP、**传输/隧道** 模式、**XFRM** 策略与 **SAD**、加解密路径。IPv6 **强制支持**；IPv4 可选。共置 HFT **通常无 IPsec 热路径**；懂 **ESP 开销与 NAT-T** 利于 **VPN/跨域** 排障。

---

## 本章概述

| | 内容 |
|---|------|
| **本章** | AH/ESP · IKE/Netkey · XFRM · ESP Rx/Tx · NAT-T |
| **前置** | [Ch 8 §3 扩展头 AH/ESP](../chapter-08-ipv6/notes/section-3-扩展头部.md) · [Ch 9 NAT](../chapter-09-netfilter/) |
| **HFT 读法** | 交易 VLAN **明文低延迟**；IPsec 在 **管理/备份链路** 或 **合规隧道** |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. IPsec 基础与操作模式 | [notes/section-1-IPsec基础与操作模式.md](./notes/section-1-IPsec基础与操作模式.md) |
| 2. IKE 与密码学 | [notes/section-2-IKE与密码学.md](./notes/section-2-IKE与密码学.md) |
| 3. XFRM 框架 | [notes/section-3-XFRM框架.md](./notes/section-3-XFRM框架.md) |
| 4. IPv4 ESP 实现 | [notes/section-4-IPv4-ESP实现.md](./notes/section-4-IPv4-ESP实现.md) |
| 5. 收发路径（传输模式） | [notes/section-5-收发IPsec数据包-传输模式.md](./notes/section-5-收发IPsec数据包-传输模式.md) |
| 6. NAT 穿越 NAT-T | [notes/section-6-IPsec中的NAT穿越.md](./notes/section-6-IPsec中的NAT穿越.md) |

---

## 相关章节

- 上一章：[../chapter-09-netfilter/](../chapter-09-netfilter/)
- 下一章：[../chapter-11-第4层协议.md](../chapter-11-第4层协议.md)
- IPv6 扩展头：[../chapter-08-ipv6/notes/section-3-扩展头部.md](../chapter-08-ipv6/notes/section-3-扩展头部.md)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
