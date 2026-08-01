# 8. 与 Ch 8 / Ch 7 的下钻链

```
应用慢？
  ├─ Ch 8 fileslower / cachestat  → 逻辑 I/O / 缓存
  ├─ Ch 7 drsnoop / swapin        → 回收 / swap 导致写盘
  └─ Ch 9 biolatency / biostacks    → 块层延迟与发起栈
```

**Gregg 经典组合：**

```
cachestat  命中低  →  预期 biolatency 右移
biolatency 长尾   →  biosnoop 抓 outlier
biostacks          →  定位 journal / swap / 应用 write
```

---
