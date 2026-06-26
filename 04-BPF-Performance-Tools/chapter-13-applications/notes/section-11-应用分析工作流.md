# 11. 应用分析工作流

```
1. 业务指标（P99、QPS）锁定 PID / 时间窗
2. syscount 10s        → futex? sched_yield? read?
3. profile 30s         → On-CPU 热点函数
4. offcputime 30s      → Off-CPU 等锁/I/O/sleep?
5. 资源章交叉验证
      futex 多 → pmlock/pmheld
      read 多 → ioprofile + Ch 8/9
      recv 多 → Ch 10 tcpretrans
6. 有 USDT → mysqld_qslower 类慢路径工具
7. naptime → 排除人为 sleep
```

---
