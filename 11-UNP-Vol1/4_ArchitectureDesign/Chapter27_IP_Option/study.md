# 第 27 章：IP 选项（厚版）

> [Ch 19 PF_KEY](../Chapter19_KeyManageSocket/study.md) · **Ch 27**（`4_ArchitectureDesign`）· [Ch 28](../../3_DeepMaster/Chapter28_RawSocket/)（待笔记）  
> 逐节：`27.x_*.md`

> **说明**：上传资料截至第 8 章；第 27 章框架来自目录（约第 13 页），细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

理解 **IPv4 IP_OPTIONS/TLV**、源路由禁令、**IPv6 扩展首部链**、**inet6_opt_*/inet6_rth_***、**粘附选项 vs sendmsg 辅助数据** 及 RFC 3542 与历史 API。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 27.1 | [27.1_Overview](./27.1_Overview.md) | IPv4 选项 vs IPv6 扩展首部 |
| 27.2 | [27.2_IPv4_Packet_Option](./27.2_IPv4_Packet_Option.md) | **IP_OPTIONS**、RR |
| 27.3 | [27.3_IPv4_Source_Route_Option](./27.3_IPv4_Source_Route_Option.md) | SSRR/LSRR、封杀 |
| 27.4 | [27.4_IPv6_Extend_Header](./27.4_IPv6_Extend_Header.md) | 菊花链顺序 |
| 27.5 | [27.5_IPv6_Hop_Dest_Option](./27.5_IPv6_Hop_Dest_Option.md) | **inet6_opt_*** |
| 27.6 | [27.6_IPv6_Route_Header](./27.6_IPv6_Route_Header.md) | **inet6_rth_***、Type0 废弃 |
| 27.7 | [27.7_IPv6_Sticky_Option](./27.7_IPv6_Sticky_Option.md) | 粘附 vs 辅助数据 |
| 27.8 | [27.8_Historical_IPv6_API](./27.8_Historical_IPv6_API.md) | RFC 2292 → 3542 |
| 27.9 | [27.9_Summary](./27.9_Summary.md) | 全章收束 |

---

## 一章速记

```text
IPv4：IP_OPTIONS 手工 TLV；RR 最多约 9 跳；源路由被全网 drop
IPv6：Hop-by-Hop(首) → Dest → Routing → Fragment → AH/ESP → 上层
构建：inet6_opt_init/append/finish；解析：opt_next/find
路由：inet6_rth_*；Type 0 已废(RFC5095)
单包：sendmsg msg_control；全局：setsockopt IPV6_DSTOPTS 等粘附
用 RFC 3542，勿用 RFC 2292 旧 API
```

---

## 与前后章挂钩

| 章节 | 关联 |
|------|------|
| Ch 7 | getsockopt/setsockopt |
| Ch 12 | IPv6 地址与双栈 |
| Ch 14 | **sendmsg** 辅助数据 |
| Ch 19 | IPsec AH/ESP 在扩展首部链中 |
| Ch 28 | ping/traceroute 用 IP 选项/ICMP |
