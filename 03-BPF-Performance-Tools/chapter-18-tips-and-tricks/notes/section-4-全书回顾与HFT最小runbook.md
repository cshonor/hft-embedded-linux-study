# 4. 全书回顾与 HFT 最小 runbook

**Part I：** 懂 BPF（Ch 1–5）  
**Part II：** 按资源查（Ch 6–16）  
**Part III：** 生态 + **心法（Ch 17–18）**

**HFT incident 最小集（遵守 Ch 18 纪律）：**

```
1. 业务 histogram 锁定段
2. Linux 60s（Ch 3）
3. runqlat 10s · tcpretrans 30s · profile -F 99 30s（Ch 6/10）
4. 若 CPU 不高 → offcputime（Ch 13）
5. bpftrace 验证单点（Ch 5）— 短跑
6. 全程：限 PID · 限时 · 热路径核慎挂
```

**构建链 checklist（Ch 18 + 12 + 13）：**

- [ ] `-fno-omit-frame-pointer`  
- [ ] debuginfo 可装  
- [ ] 无高频 uretprobe（Go）  
- [ ] 热路径无 `trace`/printf 式 BPF  

---
