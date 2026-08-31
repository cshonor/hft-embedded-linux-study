# 5.18 小结

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.18 节（印刷 p190）

## 内容详解

原书小结：

1. bpftrace 是强大的跟踪器，**高级编程语言十分简洁**；
2. 本章覆盖：特点、工具、单行程序示例；
3. 编程语言细节：**探针、控制流、变量、函数**；
4. 最后：调试与内部运作。

后续章节进入 Part II——按性能目标（观测对象）逐章展开，BCC 与 bpftrace 工具一起讲。bpftrace 的一大优势：**代码简洁到可以全书全文引用**。

### 本章一张图：你此刻在工具箱里的位置

```
观测需求
   │
   ├─ 临时验证假设（分钟级周转）────────► bpftrace 单行/短脚本   ← 本章
   │                                      代价：表达力封顶（无浮点/512B栈/无自由循环*）
   │
   ├─ 固化观测工具/守护进程 ────────────► BCC（ch04）/ 现代替代 libbpf-tools CO-RE
   │
   ├─ 全量落盘离线分析 ─────────────────► perf record / LTTng
   │
   └─ 高频函数计数 / 无 LLVM 环境 ──────► Ftrace

   * 老内核下；5.3+ 有界循环已解
```

### 本章语言速查（考试级）

| 元素 | 要点 |
|------|------|
| 程序结构 | `probe /filter/ { actions }`（类 awk） |
| 探针 | 表 5-2 十二类；通配符 + `-l` 预览；BPFTRACE_MAXPROBES=512 |
| 计时模板 | kprobe 存 `@start[tid]=nsecs` → kretprobe `/@start[tid]/` 求差 + **delete** |
| 变量 | 内置（pid=tgid！）/ `$` 临时（块内）/ `@` 映射表（类型首次赋值定） |
| 聚合 | count/sum/avg/min/max/stats/hist/lhist；print(@m, top, div) |
| 采样 | profile:hz:99（全 CPU）/ interval:s:1（单 CPU，配 exit() 限时长） |
| 控制流 | 无 else if、无无限循环；unroll(≤20 常量)；5.3+ 才有有界循环 |
| 陷阱高发 | sum 前滤负值；先存纳秒打印时除；str 64B/200B 上限；system 需 --unsafe |

### 陷阱四天王（全章报错率最高）

| # | 陷阱 | 一行修复 | 出自 |
|---|------|---------|------|
| 1 | 双探针计时不加出口过滤 | kretprobe 加 `/@start[tid]/` | 5.5/5.17 |
| 2 | sum/avg 混入 -errno | `/args->ret > 0/` | 5.5/5.11 |
| 3 | 无符号回绕当成系统离群 | 离群值 ≈ uptime 纳秒 → 查过滤器 | 5.11/5.17 |
| 4 | map 键 4096 静默截断 | 条目数恰为 4096 → 调 MAP_KEYS_MAX | 5.8 |

### BCC vs bpftrace 全维对照（收束 ch04+ch05）

| 维度 | BCC | bpftrace |
|------|-----|----------|
| API | 双执行域（BPF C + Python） | 单语言 |
| 代码量（同等观测） | ~10× | 1× |
| 表达力上限 | 高（完整 C 方言+Python 生态） | 低（刻意小语言） |
| 报错风格 | 编译错（C）+运行错（Python）双轨 | 语义错友好 / 验证器错晦涩 |
| 复用关系 | — | **复用 libbcc/libbpf + LLVM**（同源版本约束） |
| 适合 | 固化工具、守护进程 | 临时单行、快速验证、教学 |
| 现代趋势 | libbpf-tools CO-RE 正在取代其 Python 工具位 | 仍是交互排障首选前端 |

## HFT 落地建议

1. 把 5.5 的 12 条单行 + 双探针计时模板练到肌肉记忆——这是排障的"识字量"；
2. 所有自动化脚本三要素：`interval+exit` 限时长、`/过滤/` 在内核态、输出映射表命名带单位（us/ns/bytes）；
3. 进阶阅读顺序：附录 A（单行宝典）→ 第 6 章起按资源域学工具，读每章 bpftrace 源码学惯用法。

<details>
<summary>自测题</summary>

1. 用三行以内写出"统计任意内核函数耗时的微秒直方图"模板。
   <details><summary>答案</summary>kprobe:fn { @start[tid]=nsecs; } kretprobe:fn /@start[tid]/ { @us=hist((nsecs-@start[tid])/1000); delete(@start[tid]); }</details>

2. bpftrace 相比 BCC 最突出的语言特点？
   <details><summary>答案</summary>极简——同一能力代码量约为 BCC 的 1/10，短到可全文印在书里。</details>

3. map 聚合表输出恰好停在 4096 行，哪个环境变量、什么机理？
   <details><summary>答案</summary>BPFTRACE_MAP_KEYS_MAX 默认 4096；达到上限后新键静默丢弃（哈希表插入失败不报错）。</details>

4. 本章与 ch04 的复用关系一句话？
   <details><summary>答案</summary>bpftrace 底层复用 libbcc/libbpf + LLVM 完成插桩/加载/编译——与 BCC 同源，因此共享版本约束与验证器排障手段（dmesg、-v 看字节码）。</details>
</details>
