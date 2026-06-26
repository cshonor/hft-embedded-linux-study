# 5. 两套清单如何串联

```
Incident / 延迟报警
        │
        ▼
  Workload 表征（业务指标：哪段、多大、何时）
        │
        ▼
  Linux 60 秒（uptime → vmstat → mpstat → sar …）
        │  粗方向：CPU？网？盘？内存？
        ▼
  USE 下钻（对嫌疑资源查 U/S/E）
        │
        ▼
  BCC 清单（runqlat / tcpretrans / biolatency / profile …）
        │
        ▼
  bpftrace ad hoc（验证单点假设）→ 附录 A
        │
        ▼
  修复 + 回归 baseline
```

**原则：** **先消除不必要工作** → **方法论定方向** → **传统工具 60 秒** → **BCC 聚合** → **bpftrace 补洞**。

---
