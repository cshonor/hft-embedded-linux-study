# 10.15 总结

> 章级精读：[../study.md#ch10-exam](../study.md#ch10-exam)

## 本节核心目标

收束 UDP 轻量 vs IP 分片代价。

---

## 必背三条

1. **UDP**：无连接、报文边界、无拥塞控制。  
2. **避免 IP 分片**：控制单报 **≤ ~MTU**；用 PMTUD/固定 1200B（QUIC）。  
3. **接收 buffer** 要够大，防截断丢数据。

---

## 下一章

- [ch11 DNS](../../chapter11-dns-domain-resolve/study.md)
