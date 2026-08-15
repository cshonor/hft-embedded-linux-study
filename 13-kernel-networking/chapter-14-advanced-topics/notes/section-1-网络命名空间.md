# Ch 14 §1 网络命名空间 · Network Namespaces

> **Linux Kernel Networking** · Rami Rosen · **选读 🟡**

### 1. 网络命名空间 (Network Namespaces)

**Network namespace** — **轻量虚拟化**：一组进程拥有 **独立网络栈**（自己的 **`struct net`**、路由表、netfilter、设备、端口空间）。

---

## 实现机制

| 组件 | 作用 |
|------|------|
| **`unshare(CLONE_NEWNET)`** | 进程 **脱离** 原 netns，得 **新空 netns** |
| **`setns(fd, CLONE_NEWNET)`** | 加入 **已有** netns（`/proc/pid/ns/net`） |
| **`nsproxy`** | 进程 **`task_struct`** 指向 **ns 集合**（net/pid/mnt…） |
| **`struct net`** | **netns 核心** — 设备列表、sysctl、XFRM、conntrack 表… |

```
初始: 所有进程 → init_net
容器/veth: 新 netns + veth pair 一端迁入 → 独立 ip route/iptables
```

与 [Ch 9 netfilter](../chapter-09-netfilter/)、[Ch 10 XFRM](../chapter-10-ipsec/) **每 netns 隔离** 同构。

---

## 管理工具

```bash
ip netns add trading
ip netns exec trading ip link set lo up
ip link add veth0 type veth peer name veth1
ip link set veth1 netns trading
```

**`/var/run/netns/`** — `ip netns` 持久化命名。

---

## HFT

| 场景 | 读法 |
|------|------|
| **裸金属共置** | 常 **只用 init_net** — 本章 **背景知识** |
| **容器化策略/仿真** | netns **隔离测试栈** — 不与生产混用 |
| **veth 额外 hop** | **增延迟** — 生产 tick 路径 **避免** |

---

← [Ch 14](../README.md) · 下一节 [2. Cgroups](./section-2-控制组Cgroups.md)
