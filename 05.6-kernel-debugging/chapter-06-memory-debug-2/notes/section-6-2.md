# 6.2 内存调试工具组合策略

> ⬜ 跳读

## 本节要点

### 工具组合矩阵

| 阶段 | KASAN | KFENCE | SLUB debug | kmemleak | LOCKDEP |
|------|-------|--------|-----------|---------|---------|
| **开发** | ✅ | ❌ | ✅ | ✅ | ✅ |
| **测试** | ✅ | ✅ | 选 | ✅ | ✅ |
| **生产** | ❌ | ✅ | ❌ | ❌ | ❌ |
| **HFT 生产** | ❌ | ✅(低频) | ❌ | 周期 | ❌ |

### 推荐工作流

```
开发阶段:
  1. CONFIG_KASAN=y + CONFIG_UBSAN=y + CONFIG_SLUB_DEBUG=y + CONFIG_LOCKDEP=y
  2. 运行所有测试用例，修复所有 KASAN/UBSAN/lockdep 报告
  3. kmemleak 扫描确认无泄漏

测试阶段:
  1. 同开发 + CONFIG_KFENCE=y (sample_interval=100)
  2. 压力测试 + 模糊测试
  3. 验证 KFENCE 在低开销下是否发现新问题

生产阶段:
  1. CONFIG_KFENCE=y (sample_interval=1000, 更低频率)
  2. 定期 kmemleak 扫描 (非实时)
  3. lockdep 仅在 staging 环境启用
```

### HFT 特殊考虑

| 工具 | HFT 适用性 | 原因 |
|------|-----------|------|
| KASAN | ❌ 生产 | 2-3x slowdown 破坏实时性 |
| KFENCE | ✅ 生产(低频) | ~1% 开销可接受 |
| SLUB debug | ❌ 生产 | 增加分配延迟 |
| kmemleak | ⚠️ 离线 | 扫描时暂停分配，影响延迟 |
| LOCKDEP | ❌ 生产 | 每次加锁额外检查 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 KASAN 不适合 HFT 生产环境？

> KASAN 对每次内存访问插入影子内存检查（INLINE 模式 3-5 条额外指令），整体性能下降 2-3 倍。HFT 对纳秒级延迟敏感，2-3x slowdown 不可接受。KFENCE ~1% 开销是可接受的折中。

**Q2:** 为什么 kmemleak 不适合 HFT 运行时使用？

> kmemleak 扫描时需要遍历所有内存查找指针引用，扫描期间可能暂停内存分配（RCU 停顿），导致延迟毛刺。建议在非交易时段（如收盘后）手动触发扫描，而非默认的每 10 分钟自动扫描。

</details>
