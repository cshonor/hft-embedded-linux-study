## HFT 版「Unexplained Win」演练模板

> **定位：** 把 Ch 16 案例研究的方法论，压成一份 **可在 HFT 生产机直接照抄执行的 runbook**——每个步骤给命令、给判读、给「继续/排除」的分流条件。
> **HFT 实操要点：** 演练场景是**订单路径 tick→order P99 从 8 µs → 5 µs**。真发生时最常见的错误是「先庆祝、后遗忘」；正确姿势是 **只读排查 + 当天出结论**——win 的现场（邻居、缓存、频率状态）比 regression 现场更易蒸发。

```
  总决策树（每步只读，按成本递增）
  ┌─ S0 陈述+对照 ──→ 分母变了吗？（tick 量/合约集/打点 diff）
  │      └─ 变 → 测量/负载类结局，收工
  ├─ S1 统计 ────────→ 邻居/steal/runq 同变吗？
  │      └─ 变 → 环境类结局（邻居/宿主机），容量数据打标，收工
  ├─ S2 配置 diff ───→ 有 drift 吗？
  │      └─ 有 → 定位参数 → 测试机 A/B 验证 → 固化，收工
  ├─ S3 PMC ─────────→ GHz 变了吗？IPC 变了吗？哪类 miss 少了？
  │      └─ GHz 变 → 频率假象结局；IPC 不变 → 口径结局
  └─ S4 追踪 ────────→ perf diff 哪个函数？offcputime 哪个栈消失？
         └─ 函数级证据 → 真实优化结局，写 release note
```

---

### 场景设定

- **系统：** 自研策略执行进程 `strat-exec`（8 线程，绑核 8–11，NUMA 节点 0），行情→订单热路径。
- **现象：** 昨日 14:30 部署 v2.8.1 后，`tick→order P99` 从 8.1±0.3 µs 降至 5.2±0.2 µs（样本量 >10⁶）。
- **对照：** 未部署的兄弟进程 `strat-sim`（同机，核 4–7）P99 不变——**这是天然对照组**。
- **约束：** 交易时段不重启、不变更、不加载任何带写副作用的工具。

---

### S0 陈述 + 对照组（5 分钟）

```
[陈述] strat-exec v2.8.1 部署后 tick→order P99 8.1→5.2 µs（-36%），
       起始 14:30 与部署对齐；同机 strat-sim 无变化；
       对照基线：上周同交易日同时段直方图存档。
[假设矩阵]
  H1 v2.8.1 代码热路径变短（commit diff 含 risk-check 重构）
  H2 行情量/合约集合变化（分母变化）
  H3 THP/页表状态变化（昨夜 compaction）
  H4 打点口径变化（v2.8.1 动过 telemetry 代码）
  H5 CPU 频率/thermal 状态变化
  H6 邻居变化（strat-sim 没变，但同节点其他进程？宿主机？）
```

**分流：** tick 量与打点 diff 先查——

```bash
# 分母：上游 tick 速率前后对比（应用 counter / Prometheus）
# 打点：git diff v2.8.0..v2.8.1 -- src/telemetry/ src/latency_probe/
```

判读：tick 速率持平、打点未动 → H2/H4 淘汰，进 S1。**若直方图是整体平移而非尾部压缩 → 高度怀疑口径，回 H4 深挖。**

---

### S1 统计（10 分钟）：邻居与全核对照

```bash
mpstat -P ALL 1 5           # 全核利用率：核 4-7（sim）、12+（其他）有没有同时变闲？
vmstat 1 5                  # cs（上下文切换）前后对比
sar -u -f /var/log/sa/sa30  # 案发日 vs 上周同日（steal/idle）
```

| 观测 | 若为真 | 指向 |
|------|--------|------|
| 同节点其他核利用率同步下降 | H6 邻居撤出 → 环境 win | 结局：**容量数据打标** |
| %steal 从 >0 变 0 | 宿主机邻居（云） | 同上 |
| cs 显著下降 | 调度/阻塞结构变化 | 进 S3 验证（context-switches 事件） |
| 一切平静 | 便宜环境假设淘汰 | 进 S2 |

---

### S2 配置 diff（10 分钟）

```bash
diff /var/lib/perf-snap/20260829-0930/sysctl.txt \
     /var/lib/perf-snap/20260830-1430/sysctl.txt          # 部署前后快照
cat /sys/kernel/mm/transparent_hugepage/enabled           # THP 状态
taskset -pc $(pidof strat-exec)                           # 亲和事实（不是配置声明）
grep Cpus_allowed /proc/$(pidof strat-exec)/status
cat /sys/devices/system/cpu/cpu8/cpufreq/scaling_governor # governor 事实
```

| Diff 发现 | 动作 |
|-----------|------|
| 无差异 + THP=always + 亲和正确 + governor=performance | 配置假设全淘汰，进 S3 |
| 发现 drift（如 affinity 从错配变正确） | 测试机 A/B 复现该 drift → 确认因果 → **固化进部署脚本** → 结局：配置 drift |

> 本演练假设 diff 干净——「上次 affinity 脚本 path 写错、这次修复」这种经典结局留给真实世界。

---

### S3 PMC（15 分钟）：频率假象与效率真身

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,\
branch-instructions,branch-misses,context-switches,cpu-migrations,\
page-faults -p $(pidof strat-exec) -- sleep 10
```

**判读顺序（不可颠倒）：**

```
① GHz 行：4.19 GHz 恒定        → 频率假象出局（H5 淘汰）
② instructions：持平           → 干的活没变（H2 再确认淘汰）
③ IPC：1.9 → 2.4（+26%）       → 效率型 win，stall 减少
④ cache-misses：-58%           → 数据供给变好（LLC miss 大降）
⑤ branch-misses：0.3%→0.28%    → 基本没变，代码分支故事弱
⑥ context-switches：-31%       → 阻塞减少（与 vmstat cs 互证）
⑦ cpu-migrations：0            → 一直绑得很好（H-调度 淘汰）
```

**当前最强故事：** 「IPC↑ + cache-miss↓ + cs↓」——数据供给改善 + 阻塞减少。代码没变（v2.8.1 的 risk-check 重构恰好涉及数据结构？）→ **两条竞争路径**：

- A：v2.8.1 重构改善了 locality（H1 代码论）
- B：某环境状态变化改善了 cache（H3/H6 残余论）

进 S4 用 perf diff 裁决。

---

### S4 追踪（20 分钟）：路径级裁决

```bash
# on-CPU profile（采样率压低，减少扰动）
perf record -F 49 -g -p $(pidof strat-exec) -- sleep 10
perf archive new.perf.data                     # 存档供日后 diff
perf diff baseline-v2.8.0.perf.data new.perf.data

# 阻塞侧：等什么？（cs 下降的机制确认）
sudo offcputime-bpfcc -p $(pidof strat-exec) 10

# 调度尾巴：runqlat 直方图（对比案发前存档）
sudo runqlat-bpfcc 10
```

**演练设定的裁决结果（教学用）：**

```
perf diff 关键行：
  -9.8%  6.2%→0.0%  risk_check_cluster_naive()      ← v2.8.1 重构删掉的旧函数
  +1.2%  0.0%→1.2%  risk_check_cluster_soa()        ← 新实现：结构体数组→数组的结构体
offcputime：futex_wait 栈从 31% 降到 4%              ← risk-check 分片锁争用消失
runqlat：尾部 100µs bump 消失                        ← 与 cs 下降互证
```

**因果链（四问收口）：**

1. **什么变了：** v2.8.1 的 risk-check 数据结构重构（AoS→SoA + 分片锁优化）。
2. **哪一层受益：** L1/L2 locality（cache-miss↓）→ IPC↑；锁争用消失 → cs↓ → 抖动源移除。
3. **证据：** perf diff 符号级增减 + offcputime futex 栈消失 + PMC 四信号互证 + 对照组 sim 不变。
4. **分类：** **真实代码优化**（五类结局之 ⑤）——可复现、可迁移。

---

### Release Note（可交付形态）

```markdown
### v2.8.1 性能回归报告（正向）
- 指标：tick→order P99 8.1 → 5.2 µs（-36%），n>10⁶，对照组 strat-sim 无变化
- 根因：risk-check AoS→SoA 重构（commit a1b2c3d）+ 分片锁改造（commit e4f5a6b）
- 机制：L1/L2 miss -58%（PMC）；futex 阻塞占比 31%→4%（offcputime）；
        involuntary cs -31%
- 验证：重启后 win 保持（排除缓存/预热假象）；3 台同类机灰度复现一致
- 附档：perf archive（新 baseline）；排查日志（S0–S4 全程）
- 注意：v2.8.0 的 risk_check_cluster_naive 路径已删除，回滚需连回锁改造一起
```

---

### 变体场景速查（同一套模板的不同出口）

| 变体 | 走到哪一步出口 | 结局类型 | 特征判据 |
|------|----------------|----------|----------|
| 共置邻居撤出 | S1（`mpstat` 同节点核同变闲） | 环境 win | 对照组也变好；容量数据打标 |
| THP 昨夜合页 | S2（`smaps` AnonHugePages↑） | 私有状态 | 重启即失效；显式 madvise 固化 |
| governor 修复 | S2（scaling_governor 变化） | 配置 drift | 测试机 A/B 复现；固化脚本 |
| 频率假象 | S3（GHz 行↑） | 假 win | cycles↓ 但 IPC 不变 |
| 打点挪位 | S0（直方图整体平移） | 测量 artifact | 打点 diff 非空 |
| 上游降流 | S0（tick 速率下降） | 负载变化 | 吞吐同降；别庆祝 |

---

### 与仓库其他模块的接口

- 延迟测量口径（直方图形状审查、打点纪律）：[14-HFT ch09 延迟测量与基准](../../../14-hft-engineering/chapter-09-latency-measurement-benchmarking/README.md)
- PMC/软件事件原理：[16.1.5–16.1.6](./section-16.1.5-16.1.6-PMC-与软件事件.md)
- 工具手册：[Ch 13 perf](../../chapter-13-perf/) · [Ch 14 Ftrace](../../chapter-14-ftrace/) · [Ch 15 BPF](../../chapter-15-bpf/) · [附录 C bpftrace 单行](../../appendix-C-bpftrace单行命令.md)
- 内核侧根因（cache/THP/锁的机制深挖）：[06-linux-mm 模块](../../../06-linux-mm/)

---

<details>
<summary>代码自测（Q&A，先遮住答案想）</summary>

**Q1：为什么对照组 strat-sim 在 S0 就要确认，而不是最后再看？**

A：对照组是假设空间的**第一刀**：sim 不变 → 变化与部署强相关（进程级原因）；sim 也变好 → 嫌疑上移到机器/邻居/宿主机（环境级）。这刀切在 S0，直接决定 S1 的观测重点（若环境级，`mpstat` 全核和 steal 是主战场；若进程级，可直接跳配置 diff + PMC）。晚看对照组 = 前面步骤可能白做。

**Q2：S3 判读顺序为什么「GHz 先于 IPC、IPC 先于 miss 明细」？**

A：因果依赖关系：GHz 是 cycles 语义的前提（频率变则 cycles 数没有可比性）→ 必须最先排除；IPC 是「效率变了没有」的总判定 → 决定后续 miss/branch 明细有没有分析价值（IPC 不变时，miss 明细的波动可能只是噪声）。顺序颠倒会被局部 miss 变化带偏，得出「解释了没发生的事」的结论。

**Q3：perf diff 显示某函数占比从 6.2% 降到 0，为什么还必须配 PMC 和对照组才能下「真实优化」结论？**

A：占比是相对数——6.2%→0 也可能是**别的函数变慢稀释**（分母变大）或**调用方逻辑变化跳过了它**（比如错误路径提前 return，功能其实缺失）。PMC（instructions 持平 + IPC↑ + miss↓）证明总工作量没变而效率升；对照组排除机器级原因。三件套齐了，因果链才闭合。

**Q4：演练里为什么重启实验放在 release note 的「验证」栏而不是排查主路径？**

A：重启实验是**裁决性反证**（区分代码/状态），但生产机交易时段不可重启——它只能安排在盘后或灰度机执行。排查主路径（S0–S4）全程只读、可当场完成；重启验证作为结论的最后一道保险（排除缓存/预热假象）补进 release note。**流程设计要区分「当场能做的」和「要排期的」**。

**Q5：如果 S0–S4 全部走完仍是「unexplained」——所有便宜假设淘汰、PMC 显示 IPC↑ cache-miss↓、但 perf diff 干净、配置 diff 干净——下一步怎么办？**

A：进入了真 unexplained 区，通常意味着观测范围外有变量。按顺序补：① 私有状态面：`smaps` 前后对比（THP/页表）+ 重启实验排期；② 环境面：云宿主机更换的侧面证据（microcode、dmesg 启动时间、`/proc/cpuinfo` 步进）；③ 编译面：二进制本身 diff（同源码不同编译环境的布局差异，如 PGO/LTO/地址布局）；④ 记录在案——「未解释但已归档」的 win 也是资产，等下次同类 win 出现时数据可叠加。承认未知本身是方法论的一部分。

</details>

---

← [本章导读](../README.md)
