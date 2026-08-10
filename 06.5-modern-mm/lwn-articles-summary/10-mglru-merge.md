# MGLRU 合入 6.1

> **原文:** [MGLRU merged for 6.1](https://lwn.net/Articles/913685/) (LWN, 2022)
> **内核版本:** 6.1 (合入主线)
> **对标旧书:** ULK3 Ch17 (传统 LRU 已过时)

---

## 核心观点

MGLRU 经过 3 年开发和测试，最终在 Linux 6.1 合入主线。Google 在 Chrome OS 和 Android 上验证了显著性能提升。

### 合入数据

| 场景 | 传统 LRU | MGLRU | 改善 |
|------|---------|-------|------|
| Chrome OS 内存压力 | 基准 | 减少 90% 页扫描 | CPU 占用 -20% |
| Android 冷启动 | 基准 | -15% 启动时间 | 更少 I/O |
| 服务器 (大内存) | 基准 | kswapd CPU -40% | 尾延迟改善 |
| 数据库 (PostgreSQL) | 基准 | +5% TPS | 缓存命中率更高 |

### 合入过程

```
2019: Yu Zhao 开始开发 MGLRU
2021: 首次提交 LKML
2022.3: 合入 linux-next 测试
2022.10: Linus 接受合入 6.1
```

### 争议与解决

| 争议 | 解决 |
|------|------|
| 代码复杂度高 (~3000 行) | 通过编译选项 CONFIG_LRU_GEN 控制，不启用零影响 |
| 与 cgroup v2 内存回收交互 | 扩展 memcg LRU 支持 MGLRU |
| 调试困难 | 添加 debugfs 接口 |
| 部分 maintainer 反对 | Google 生产数据说服 |

### 配置

```bash
# 内核配置
CONFIG_LRU_GEN=y        # 启用 MGLRU
CONFIG_LRU_GEN_ENABLED=y  # 默认启用 (运行时可关)
CONFIG_LRU_GEN_STATS=y    # 统计信息

# 运行时控制
echo y > /sys/kernel/mm/lru_gen/enabled    # 启用
echo n > /sys/kernel/mm/lru_gen/enabled    # 禁用 (回退到传统 LRU)
```

---

## 与旧书差异

| ULK3 讲的 | 现代实现 |
|-----------|---------|
| 传统 2 代 LRU | MGLRU 多代 LRU (6.1+) |
| `shrink_lruvec()` | `lru_gen_look_around()` |
| 无运行时开关 | `lru_gen/enabled` 可运行时切换 |

---

## HFT 关联

MGLRU 减少 kswapd CPU 占用 40%，间接为交易线程留出更多 CPU 时间。HFT 系统建议启用 MGLRU（虽然 mlockall 后实际回收少，但 kswapd 扫描开销降低有益）。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** MGLRU 合入后，传统 LRU 代码是否被删除？

> 没有。MGLRU 和传统 LRU 共存，通过 CONFIG_LRU_GEN 编译选项选择。不启用 MGLRU 时，内核使用传统 LRU 代码。运行时也可以通过 `/sys/kernel/mm/lru_gen/enabled` 禁用 MGLRU 回退到传统 LRU。

</details>
