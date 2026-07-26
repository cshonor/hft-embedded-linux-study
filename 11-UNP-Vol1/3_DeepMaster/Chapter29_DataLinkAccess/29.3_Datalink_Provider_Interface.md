# 29.3 DLPI：数据链路提供者接口

---

## 核心主旨

**DLPI（Datalink Provider Interface）** — **SVR4 / Solaris** 等的数据链路标准。

---

## 机制

- 基于 **STREAMS**  
- 向流 **push** 模块（如 **`pfmod`** 过滤）  
- 收发需复杂控制消息（`dl_attach_req` 等）

---

## 重点结论

API **晦涩**，编程成本高 — 现代项目优先 **libpcap** 抽象，少直接写 DLPI。

---

## 个人学习总结

（待填）
