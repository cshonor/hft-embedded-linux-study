# 21.12 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 相对广播

硬件过滤 +（可选）WAN 多播树 — **降维打击**广播的「全子网纳税」。

### 2. 收发不对等

| 角色 | 要求 |
|------|------|
| **发** | 向多播地址 `sendto`；调 TTL/接口/loop；**通常不必 Join** |
| **收** | **`IP_ADD_MEMBERSHIP` / mcast_join** + IGMP/MLD；**bind 前 SO_REUSEADDR** |

### 3. 地址与碰撞

- IPv4 D 类；IPv6 `ff00::/8` + Scope  
- **32:1** MAC 映射 → IP 层二次过滤  

### 4. 安全演进

**ASM → SSM**（IGMPv3 / MLDv2）绑定合法源。

---

## 广播 vs 多播速记

| | 广播 | 多播 |
|--|------|------|
| 过滤 | 子网全体 | 仅 Join 者（+硬件） |
| WAN | 一般不转发 | 多播路由 |
| 套接字 | SO_BROADCAST | ADD_MEMBERSHIP 等 |
| IPv6 | 无 | 标准 |

---

## 个人学习总结

（待填）
