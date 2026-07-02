# Ch 3 §3 ICMP 套接字 · Ping Sockets

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 3. ICMP 套接字 ("Ping Sockets")

早期 Linux：**ping** 只能 **raw socket**（`SOCK_RAW` + `IPPROTO_ICMP`）— **必须 root/CAP_NET_RAW**。  
现代引入专用 **`IPPROTO_ICMP` 套接字类型**（源于 **Openwall** 安全补丁思路）— 支持 **setuid-less ping**。

---

## 两种用户态方式对比

| | **Raw ICMP socket** | **Ping socket (`IPPROTO_ICMP`)** |
|---|---------------------|----------------------------------|
| 权限 | `CAP_NET_RAW` / root | **普通用户**（在授权组内） |
| API | `socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)` | `socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP)` |
| 安全性 | 可构造任意 ICMP/IP | **内核代填 IP/ICMP 头**，能力受限 |
| 典型命令 | 老式 ping、部分扫描工具 | **`/bin/ping`**（现代发行版默认） |

**SOCK_DGRAM + IPPROTO_ICMP：** 用户只写 **payload（id/seq/data）** — **内核** 负责 **IP/ICMP 头** 与 **校验和**，减少 **伪造 ICMP** 攻击面。

---

## 授权：`ping_group_range`

```bash
# 查看允许使用 ping socket 的 GID 范围
cat /proc/sys/net/ipv4/ping_group_range
# 默认常见：1 0  （仅 root）或 1 65534 等发行版差异

# 例：允许 GID 100–100 的用户 ping
echo "100 100" > /proc/sys/net/ipv4/ping_group_range
```

|  sysctl | 含义 |
|--------|------|
| **`net.ipv4.ping_group_range`** | **min max** GID — 落在此区间的用户可 **无 CAP** 使用 ping socket |

**运维：** 共置机通常 **禁止无关用户 ping 外网**；监控探针可用 **专用 GID** 或 **capability** 最小化。

---

## 与内核 per-CPU ICMP sock 的关系

| 套接字 | 谁用 |
|--------|------|
| **`icmp_sk`（内核）** | `icmp_send()` **差错回复**、内核自动 Echo Reply |
| **用户 ping socket** | 用户态 **`ping`** 主动 Echo Request |

路径最终在 **`icmp_echo()` / icmp_reply`** 汇合，但 **权限模型** 分离。

→ 用户态 socket API：[09-UNP](../../09-UNP-Vol1/) · 附录 A：[appendix-A-Linux-API.md](../../appendix-A-Linux-API.md)

---

## HFT 要点

- **连通性测试** — `ping` 测的是 **ICMP 路径**，**不等于** TCP 443/行情 UDP 通。
- **防火墙 drop ICMP** — ping 失败但 **业务仍通**；反之 ping 通 **TCP 仍可能被拒**。
- **latency 测量** — ICMP RTT **≠** 应用 RTT；HFT 用 **专用探测或 TCP/UDP echo**。

---

← [2. ICMPv6](./section-2-ICMPv6的扩展与变化.md) · [Ch 3](../README.md) · 下一节 [4. iptables REJECT](./section-4-Iptables与ICMP消息的生成.md)
