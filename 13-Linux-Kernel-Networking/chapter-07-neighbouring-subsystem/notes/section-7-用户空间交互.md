# Ch 7 §7 用户空间交互 · User Space Interaction

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 7. 用户空间与邻居子系统的交互

管理员通过 **net-tools** 或 **iproute2** 查看/修改 **内核邻居表** — 推荐 **`ip neigh`**（**rtnetlink**，[Ch 2](../../chapter-02-netlink-sockets/)）。

---

## 工具对比

| 工具 | 命令 | 说明 |
|------|------|------|
| **net-tools** | `arp -n` | 仅 **IPv4**；**ioctl** 老接口 |
| **iproute2** | **`ip neigh show`** | **IPv4 + IPv6**；显示 **NUD 状态**、**lladdr** |
| iproute2 | `ip neigh add/del/change` | **静态/永久** 邻居 |

```bash
# 查看
ip neigh show dev eth0

# 静态邻居（HFT 常用）
ip neigh add 192.168.1.1 lladdr 00:11:22:33:44:55 dev eth0 nud permanent

# 删除
ip neigh del 192.168.1.1 dev eth0
```

---

## `ip neigh` 输出字段

```
192.168.1.1 dev eth0 lladdr 00:11:22:33:44:55 REACHABLE
│              │              │                      │
L3 地址        接口           MAC                   NUD 状态
```

| 关键字 | 含义 |
|--------|------|
| **`lladdr`** | 链路层地址 |
| **`nud permanent`** | **不超时** — 无 GC |
| **`nud reachable/stale/...`** | 内核 **NUD**（§6） |
| **`router`** | IPv6 **路由器** 标志 |

---

## Netlink 与持久化

- **`RTM NEWNEIGH / DELNEIGH`** — `ip` 底层消息  
- **重启丢失：** `ip neigh add … permanent` **非默认持久** — 需 **networkd/ifup 脚本** 或 **配置文件** 重放  

**HFT 上线清单：**

1. 交易 GW、行情 GW **静态 neigh**  
2. `ip neigh show | grep -v REACHABLE` **告警**  
3. 与 **ECMP 每个 nexthop** 各一条（Ch 6）  

---

## 本章小结

| 节 | 带走 |
|----|------|
| §1 | **L3→L2**；ARP vs ND |
| §2 | **`neighbour`、`arp_tbl`/`nd_tbl`、arp_queue、GC** |
| §3 | **`arp_solicit` / `arp_rcv`** |
| §4 | **NDISC 五种 ICMPv6、`ndisc_rcv`** |
| §5 | **IPv6 DAD** |
| §6 | **NUD 状态机** |
| §7 | **`ip neigh` > `arp`** |

---

## 相关章节

- 下一章：[Ch 8 IPv6](../../chapter-08-IPv6.md)
- 发送路径：[Ch 4 §5](../../chapter-04-ipv4/notes/section-5-发送IPv4数据包.md)

---

← [6. NUD](./section-6-NUD网络不可达检测状态机.md) · [Ch 7](../README.md)
