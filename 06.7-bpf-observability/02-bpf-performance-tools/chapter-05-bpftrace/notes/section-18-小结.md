# 5.18 小结

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.18 节（印刷 p190）

## 内容详解

原书小结：

1. bpftrace 是强大的跟踪器，**高级编程语言十分简洁**；
2. 本章覆盖：特点、工具、单行程序示例；
3. 编程语言细节：**探针、控制流、变量、函数**；
4. 最后：调试与内部运作。

后续章节进入 Part II——按性能目标（观测对象）逐章展开，BCC 与 bpftrace 工具一起讲。bpftrace 的一大优势：**代码简洁到可以全书全文引用**。

## 本章语言速查（考试级）

| 元素 | 要点 |
|------|------|
| 程序结构 | `probe /filter/ { actions }`（类 awk） |
| 探针 | 表 5-2 十二类；通配符 + `-l` 预览；BPFTRACE_MAXPROBES=512 |
| 计时模板 | kprobe 存 `@start[tid]=nsecs` → kretprobe `/@start[tid]/` 求差 |
| 变量 | 内置（pid=tgid！）/ `$` 临时（块内）/ `@` 映射表（类型首次赋值定） |
| 聚合 | count/sum/avg/min/max/stats/hist/lhist；print(@m, top, div) |
| 采样 | profile:hz:99（全 CPU）/ interval:s:1（单 CPU，配 exit() 限时长） |
| 控制流 | 无 else if、无无限循环；unroll(≤20 常量)；5.3+ 才有有界循环 |
| 陷阱高发 | sum 前滤负值；先存纳秒打印时除；str 64B/200B 上限；system 需 --unsafe |

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
</details>
