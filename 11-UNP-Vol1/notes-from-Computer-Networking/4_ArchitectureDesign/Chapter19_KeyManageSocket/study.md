# 第 19 章：密钥管理套接字（厚版）

> [Ch 18](../Chapter18_RoutingSocket/study.md) · **Ch 19**（`4_ArchitectureDesign`）· [Ch 20](../../3_DeepMaster/Chapter20_Broadcast/)（待笔记）  
> 逐节：`19.x_*.md`

> **说明**：上传资料截至第 8 章；第 19 章框架来自目录，细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

掌握 **IPsec / SA / SADB**、**`PF_KEY` 套接字**、**`sadb_msg` 与扩展**、**`SADB_DUMP` / `SADB_ADD`** 与 **IKE 动态流程（ACQUIRE → GETSPI → UPDATE）**。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 19.1 | [19.1_Overview](./19.1_Overview.md) | IPsec、PF_KEY、root |
| 19.2 | [19.2_Socket_Read_Write](./19.2_Socket_Read_Write.md) | **sadb_msg**、扩展对齐 |
| 19.3 | [19.3_Security_DB_Dump](./19.3_Security_DB_Dump.md) | **SADB_DUMP** |
| 19.4 | [19.4_Static_Security_Create](./19.4_Static_Security_Create.md) | **SADB_ADD** 静态 SA |
| 19.5 | [19.5_Dynamic_Security_Maintain](./19.5_Dynamic_Security_Maintain.md) | IKE、**ACQUIRE** |
| 19.6 | [19.6_Summary](./19.6_Summary.md) | 全章收束 |

---

## 一章速记

```text
socket(PF_KEY, SOCK_RAW, PF_KEY_V2)；须 root
消息：sadb_msg + sadb_sa/address/key…；长度以 8 字节为单位
SADB_DUMP：write 请求 → 循环 read 直至 seq 结束标记
静态：SADB_ADD 手工拼 SPI、地址、密钥（对齐！）
动态：无 SA 发包 → ACQUIRE → GETSPI → IKE(UDP500) → UPDATE → 发包继续
控制面(IKE) / 数据面(内核 ESP) 解耦
```

---

## 与前后章挂钩

| 章节 | 关联 |
|------|------|
| Ch 18 | 同类特殊域、结构化消息、root |
| Ch 20+ | 广播/多播等 IP 层应用 |
| [Ch 27](../Chapter27_IP_Option/study.md) | IPv6 扩展首部、AH/ESP 位置 |
