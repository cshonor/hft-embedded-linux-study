# 7.8 总结

> 章级精读：[../study.md#ch07-exam](../study.md#ch07-exam)

## 本节核心目标

收束：**Middlebox 是默认现实**，协议设计需考虑 NAT/防火墙。

---

## 开发 checklist

1. 假设客户端在 **NAPT 后**，服务端可能在 **云 NAT** 后。
2. 长连接：**Keepalive + ICE/STUN** 刷新映射。
3. 载荷勿依赖内嵌 IP（或提供 **ALG 无关** 设计）。
4. 测试 **对称 NAT** 场景下是否需 **TURN**。

---

## 下一章

- [ch08 ICMP/ND](../../chapter08-icmpv4-icmpv6/study.md)
