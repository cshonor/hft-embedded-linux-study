# 11.13 总结

> 章级精读：[../study.md#ch11-exam](../study.md#ch11-exam)

## 本节核心目标

收束 DNS 在应用层的枢纽地位。

---

## 要点

- **分布式 + 缓存 + TTL** = 可扩展命名。
- **UDP 为主、TCP 补大应答/区传**；**EDNS0** 扩展能力。
- 安全靠 **DNSSEC** 与运营硬化；高并发服务慎用阻塞 `getaddrinfo`。

---

## 下一章

- [ch12 TCP 基础](../../chapter12-tcp-basic/study.md)
