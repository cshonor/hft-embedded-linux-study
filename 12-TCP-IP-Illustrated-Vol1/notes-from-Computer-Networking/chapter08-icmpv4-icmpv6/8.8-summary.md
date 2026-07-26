# 8.8 总结

> 章级精读：[../study.md#ch08-exam](../study.md#ch08-exam)

## 本节核心目标

收束 ICMP 在 v4/v6 下的双重角色与排障清单。

---

## 一页对照

| | ICMPv4 | ICMPv6 |
|--|--------|--------|
| 差错+ping | 是 | 是 |
| 邻居解析 | ARP（ch04） | **NDP（本章）** |
| 多播管理 | IGMP | MLD |

---

## 排障 checklist

1. 路径问题：`ping` / `traceroute` / `mtr`
2. MTU：`ping -M do` / 看 **PTB**
3. v6：确认 **RA**、**NS/NA**、防火墙未拦 **ICMPv6 必要类型**

---

## 下一章

- [ch09 广播/多播](../../chapter09-broadcast-multicast/study.md)
