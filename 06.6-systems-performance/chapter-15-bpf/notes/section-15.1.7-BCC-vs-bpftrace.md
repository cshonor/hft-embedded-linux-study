## 15.1.7 BCC vs bpftrace

> 章节导航：[15.1 BCC](./section-15.1-BCC-BPF-Compiler-Collection.md) · 上一篇 ← · 下一篇 [15.2 bpftrace](./section-15.2-bpftrace.md) · [本章导读](../README.md)

**本节讲什么**：两个 BPF 前端在语言、开发速度、性能、维护、输出五个维度的系统对比，Gregg 的「先标准工具 → 即兴追 → 升格」工作流，以及 HFT runbook 里两剑怎么配合。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | **不是二选一，是双剑互补** | BCC 标准武器库 + bpftrace 现场手术刀 |
| 2 | 分界线是**复杂度** | 单探针/单行 → bpftrace；多事件状态机 → BCC |
| 3 | **升格工作流** | 重复有用的 bpftrace 脚本要升格，不要每次重写 |
| 4 | 生产危机**先跑预制工具** | 定制是第二步，不是第一步 |
| 5 | 长期产品走 **libbpf/CO-RE** | 两者的终点 |

---

### 一、五维对比表

| 维度 | **BCC** | **bpftrace** |
|------|---------|--------------|
| **语言** | Python/Lua + C BPF 程序 | 专用 DSL（类 awk） |
| **上手** | 跑预制工具快；开发慢（三件套：C + Python + 构建环境） | 单行极快；复杂脚本中等 |
| **表达力** | 任意 Python 逻辑 + 多 map 协作 + 状态机 | 单探针多 action；循环/复杂控制流受限 |
| **输出** | 成熟 CLI 格式（表/直方图/汇总） | 自定义 print/map 输出 |
| **性能** | 优化充分（直方图内核聚合等已调好） | 多数场景足够 |
| **维护** | 适合**团队标准工具**（版本化、测试、文档） | 适合**个人诊断脚本**（改一行就跑） |
| **部署依赖** | clang/LLVM + 内核头（重） | bpftrace 二进制（轻，含内嵌编译器但开箱即用） |

**判断口诀：**
- 「**这个内核函数慢不慢？**」→ bpftrace 一行
- 「**每次 X 事件，把 A 和 B 两个时间戳对齐算差值，还要按进程分桶**」→ BCC

### 二、同一任务的两副面孔：测函数延迟

bpftrace 版（单行，30 秒写完）：

```bash
sudo bpftrace -e 'uprobe:/opt/strategy:decode { @s[tid] = nsecs; }
  uretprobe:/opt/strategy:decode /@s[tid]/ { @lat = hist(nsecs - @s[tid]); delete(@s[tid]); }'
```

BCC 版（同等功能，Python + C 约 60 行，省略）能多做的是：输出格式定制、错误路径处理、`--verbose` 调试、多工具参数（-p/-d/-T）工程化、单元测试、随包发布给不写代码的同事。

**本质区别不在能不能做，而在做完之后谁维护**：一次性问题用 bpftrace；要进 runbook 给值班同学复制粘贴的，用 BCC 预制或升格版。

### 三、Gregg 升格工作流

```
1. 生产 crisis → BCC 标准工具（runqlat、tcpretrans、biolatency…）
   ↓ 不够用（没有现成探针/维度）
2. bpftrace 即兴追 kprobe/uprobe → 验证假设
   ↓ 证明这个问题会重复出现
3. 升格：bpftrace 脚本 → BCC 工具 / runbook 固化条目
   ↓ 要成为长期产品级组件
4. libbpf + CO-RE（04-BPF 专书）
```

关键纪律：**第 2 步到第 3 步的升格经常被跳过**——同一个 bpftrace 脚本每次出事重新翻出来改，三次之后就该升格进 runbook（版本化、加注释、写判读指南）。

### 四、HFT runbook：两剑配合实例

```
告警：tick-to-order P99 尖刺
  ├─ 第 0 反应（预制 BCC，无脑跑）
  │    runqlat-bpfcc 10        → 调度延迟形状？
  │    offcputime-bpfcc -p $(pidof strategy) 30  → 离开 CPU 在等什么？
  │    tcpretrans-bpfcc 30     → 通道有无重传？
  │
  ├─ 分流 A：Lock 等待 → bpftrace 追具体锁
  │    bpftrace -e 'kprobe:__mutex_lock_slowpath { @[comm] = count(); }'
  │
  ├─ 分流 B：网络 → ss -tiepm + tcpretrans（ch10）
  │
  └─ 分流 C：mystery stall（软件全盲）→ Ftrace hwlat（ch14，SMI 检测）
```

要点：
- **第 0 反应必须是预制工具**——危机时刻没有时间写脚本，runbook 里预设好可直接复制的命令（见 [ch16 HFT 演练模板 S0](../../chapter-16-case-studies/notes/section-16.9-HFT-版Unexplained-Win演练模板.md)）。
- **bpftrace 只在分流后的定点追击**——假设已经收窄到具体函数/锁，一行 kprobe 定罪。
- **每次危机后复盘**：这次用的 bpftrace 有没有第二次出现？有 → 升格。

### 五、什么时候两个都不选

| 情况 | 用什么 |
|------|--------|
| 只要计数/比率，不需要逐次 | **perf stat**（[ch13](../../chapter-13-perf/)，零开销） |
| 只要 on-CPU 热点 | **perf record**（[ch13](../../chapter-13-perf/)） |
| 要 function_graph 调用层级 | **Ftrace**（[ch14](../../chapter-14-ftrace/)） |
| 要硬件级延迟（SMI） | **Ftrace hwlat**（[ch14](../../chapter-14-ftrace/notes/section-14.9-硬件延迟检测hwlat.md)） |
| 需要跨内核版本分发二进制 | **libbpf + CO-RE**（[06.7](../../../06.7-bpf-observability/)） |

观测工具的完整选型闭环见 [ch15 README 的分工表](../README.md)。

### HFT / 嵌入式关联

- **值守同学不写代码**：runbook 的第一反应命令全部是 BCC 预制工具或封装好的脚本——即兴 bpftrace 是工程师的特权，不是值守流程的一部分。
- **升格 = 降低 MTTR**：三次手写的脚本升格成 runbook 条目后，下次危机从 10 分钟定位降到 30 秒。
- **审计口径**：runbook 里的每条 BCC/bpftrace 命令要有预期输出样例和判读注释——没有判读指南的命令等于没写。

### 衔接

- 上一节：[15.1 BCC](./section-15.1-BCC-BPF-Compiler-Collection.md)（工具地图与生命周期）
- 下一节：[15.2 bpftrace](./section-15.2-bpftrace.md)（语言语法与单行库）
- 实战场景：[ch16 HFT 演练模板](../../chapter-16-case-studies/notes/section-16.9-HFT-版Unexplained-Win演练模板.md)

---

### 常见陷阱

1. **BCC 和 bpftrace 二选一**——Gregg 强调互补双剑：生产 crisis 用 BCC 标准工具，不够再上 bpftrace。
2. **bpftrace 脚本不升格**——重复有用的 bpftrace 脚本应升格为 BCC 工具或 runbook，不是每次重写。
3. **BCC 工具不记 runbook**——出事才 man page，runbook 应预设好第一反应 BCC 命令。
4. ** crisis 时刻现场写脚本**——第一反应永远是预制工具；定制化追击是假设收窄后的事。

<details>
<summary>自测题（点击展开）</summary>

1. Gregg 的 BCC/bpftrace 工作流是什么？
   <details><summary>答</summary>1) 生产 crisis → BCC 标准工具 2) 不够 → bpftrace 即兴追 3) 证明有用 → 升格 BCC/runbook 4) 长期产品 → libbpf/CO-RE</details>
2. 为什么 bpftrace 脚本应该升格？
   <details><summary>答</summary>重复有用的脚本应升格为 BCC 工具或 runbook——避免每次出事重写，且可团队共享、可加判读指南。</details>
3. HFT runbook 中 BCC 工具应该怎么用？
   <details><summary>答</summary>预设好第一反应命令（延迟尖刺→offcputime/runqlat），复制粘贴即可跑；输出样例与判读注释一并写入。</details>
4. 「测某函数每次耗时并分桶」选哪个？
   <details><summary>答</summary>一次性验证 → bpftrace 单行（uprobe+uretprobe+hist）；要产品化给团队 → BCC Python。</details>
5. 什么时候 BPF 两剑都不如别的工具？
   <details><summary>答</summary>只要计数比率 → perf stat 零开销；on-CPU 热点 → perf record；调用层级 → Ftrace function_graph；SMI 硬件延迟 → hwlat。</details>

</details>


---

← [本章导读](../README.md)
