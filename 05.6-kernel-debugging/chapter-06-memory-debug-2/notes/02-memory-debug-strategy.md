# 6.2 内存调试工具组合策略

> ⬜ 跳读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

不同工具各有优劣，需要根据阶段（开发/测试/生产）组合使用，最大化检测覆盖同时控制开销。

## 工具组合矩阵

| 阶段 | KASAN | KFENCE | SLUB debug | kmemleak | UBSAN | LOCKDEP |
|------|-------|--------|-----------|---------|-------|---------|
| **开发** | ✅ | ❌ | ✅(FZP) | ✅ | ✅ | ✅ |
| **测试** | ✅ | ✅ | 选 | ✅ | ✅ | ✅ |
| **预生产** | ❌ | ✅ | ❌ | 周期 | ❌ | ⚠️ |
| **生产** | ❌ | ✅(低频) | ❌ | 离线 | ❌ | ❌ |
| **HFT 生产** | ❌ | ✅(1s) | ❌ | 非交易时段 | ❌ | ❌ |

## 推荐工作流

### 开发阶段

```bash
# 开发内核配置
CONFIG_KASAN=y
CONFIG_KASAN_INLINE=y
CONFIG_UBSAN=y
CONFIG_UBSAN_SIGNED_OVERFLOW=y
CONFIG_SLUB_DEBUG=y
CONFIG_DEBUG_KMEMLEAK=y
CONFIG_LOCKDEP=y
CONFIG_KCSAN=y

# Boot 参数
# slub_debug=FZP
# kmemleak=on

# 工作流:
# 1. 编译 KASAN 内核
# 2. 运行所有测试用例
# 3. 修复所有 KASAN/UBSAN/lockdep 报告
# 4. kmemleak 扫描确认无泄漏
# 5. KCSAN 检查数据竞争
```

### 测试阶段

```bash
# 测试内核配置
CONFIG_KASAN=y
CONFIG_KFENCE=y
CONFIG_UBSAN=y
CONFIG_LOCKDEP=y

# KFENCE 采样频率调高
# kfence.sample_interval=10  # 10ms（更频繁）

# 工作流:
# 1. 同开发环境 + KFENCE
# 2. 压力测试 + 模糊测试 (syzkaller)
# 3. 验证 KFENCE 在低开销下是否发现新问题
# 4. 长跑测试 (24h+) + 定期 kmemleak 扫描
```

### 生产阶段

```bash
# 生产内核配置
CONFIG_KFENCE=y
CONFIG_HARDENED_USERCOPY=y
CONFIG_STACKPROTECTOR_STRONG=y
CONFIG_RANDOMIZE_BASE=y
CONFIG_DEBUG_LIST=y
CONFIG_DEBUG_SG=y

# KFENCE 低频采样
# kfence.sample_interval=1000  # 1秒

# 工作流:
# 1. KFENCE 持续监控
# 2. 非交易时段 kmemleak 扫描
# 3. /proc/meminfo 趋势监控
# 4. 出问题时切换到 staging 环境 + KASAN
```

## HFT 特殊考虑

| 工具 | HFT 适用性 | 原因 | 替代方案 |
|------|-----------|------|---------|
| KASAN | ❌ 生产 | 2-3x slowdown 破坏实时性 | KFENCE |
| KFENCE | ✅ 生产(低频) | ~1% 开销可接受 | - |
| SLUB debug | ❌ 生产 | 增加分配延迟 | - |
| kmemleak | ⚠️ 离线 | 扫描时暂停分配 | 非交易时段扫描 |
| LOCKDEP | ❌ 生产 | 每次加锁额外检查 | Staging 环境启用 |
| KCSAN | ❌ 生产 | 插桩开销 | Staging 环境启用 |
| UBSAN | ❌ 生产 | 2-5% 开销 | 开发环境启用 |

## 开销详细对比

```
工具开销排序（从低到高）:
1. /proc/meminfo          0%
2. KFENCE (1s interval)   <0.1%
3. KFENCE (100ms)         ~1%
4. DEBUG_LIST/DEBUG_SG    <1%
5. UBSAN                  2-5%
6. LOCKDEP                5-10%
7. KCSAN                  10-20%
8. SLUB debug (FZP)       10-20%
9. KASAN (INLINE)         200-300%
```

## HFT 生产环境推荐配置

```bash
# HFT 生产内核配置（安全 + 低开销）
CONFIG_KFENCE=y
CONFIG_HARDENED_USERCOPY=y
CONFIG_STACKPROTECTOR_STRONG=y
CONFIG_RANDOMIZE_BASE=y
CONFIG_DEBUG_LIST=y
CONFIG_DEBUG_SG=y
CONFIG_REFCOUNT_FULL=y
CONFIG_PANIC_ON_OOPS=y
CONFIG_CRASH_DUMP=y
CONFIG_KEXEC=y

# Boot 参数
# kfence.sample_interval=1000
# panic_on_oops=1
# kdump 配置好
```

## 问题定位工作流

```
生产环境 KFENCE 报告内存错误:
  ↓
切换到 staging 环境
  ↓
启用 KASAN 复现
  ↓
KASAN 报告精确位置（文件:行号）
  ↓
修复代码
  ↓
开发环境验证（KASAN + UBSAN + kmemleak）
  ↓
测试环境验证（压力测试 + KFENCE）
  ↓
部署到生产环境
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 KASAN 不适合 HFT 生产环境？

> KASAN 对每次内存访问插入影子内存检查（INLINE 模式 3-5 条额外指令），整体性能下降 2-3 倍。HFT 对纳秒级延迟敏感，2-3x slowdown 不可接受。KFENCE ~1% 开销是可接受的折中。

**Q2:** 为什么 kmemleak 不适合 HFT 运行时使用？

> kmemleak 扫描时需要遍历所有内存查找指针引用，扫描期间可能暂停内存分配（RCU 停顿），导致延迟毛刺。建议在非交易时段（如收盘后）手动触发扫描，而非默认的每 10 分钟自动扫描。

**Q3:** KASAN + KFENCE + SLUB debug 三者如何组合使用？

> 开发环境：KASAN + SLUB debug（poison + tracking）+ KFENCE 全开，最大化检测覆盖。测试环境：KASAN + KFENCE（采样）。生产环境：仅 KFENCE（开销 <1%），不启用 KASAN。注意 KASAN 和 SLUB redzone 重叠时以 KASAN 为主。

**Q4:** HFT 生产环境发现 KFENCE 报告后如何处理？

> (1) 记录 KFENCE 报告（dmesg + kdump）；(2) 切换到 staging 环境；(3) 启用 KASAN 复现问题（精确到文件:行号）；(4) 修复代码；(5) 开发环境验证（KASAN + UBSAN + kmemleak）；(6) 测试环境压力测试；(7) 部署修复到生产。

**Q5:** LOCKDEP 在 HFT 生产环境是否可以开启？

> 通常不建议。LOCKDEP 开销约 5-10%，对 HFT 热路径有影响。建议：初次部署时在 staging 环境启用 LOCKDEP 跑一段时间，确认无锁问题后生产环境关闭。或只在特定调试期间临时开启。

</details>

## 交叉引用

- [05.6 ch05 KASAN](../../chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch06 KFENCE](../../chapter-06-memory-debug-2/notes/01-kfence.md)
- [05.6 ch05 kmemleak](../../chapter-05-memory-debug-1/notes/05-kmemleak.md)
