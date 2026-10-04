# 5.17 bpftrace 的调试

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.17 节（印刷 p186–189）

## 内容详解

定位问题先看 **第 18 章**（事件丢失、调用栈残缺、符号不完整等常见问题）。

**与 BCC 的本质差异**：bpftrace 由一组稳定的、设计上可安全共存的功能集组成，倾向失败时弹**用户友好错误消息**，一般不需进一步调试；BCC 允许 C/Python 自由发挥，能力更广但不保证协同工作，更常需要调试模式。

一句话分工：**bpftrace 调"逻辑 bug"（用 printf），BCC 调"一切"（逻辑+集成+环境）**。

### 5.17.1 printf 调试

经典 bug 现场——vfs_read 计时直方图出现**超高离群点**，你能定位吗？

```awk
kprobe:vfs_read  { @start[tid] = nsecs; }
kretprobe:vfs_read
{
    $duration_ms = (nsecs - @start[tid]) / 1000000;
    @ms = lhist($duration_ms, ...);
    delete(@start[tid]);
}
```

问题：**过滤器缺失**。bpftrace 启动时已在执行中的 vfs_read（入口没记录），其 kretprobe 仍触发，`@start[tid]` 未初始化 = 0 → `nsecs - 0` = 巨大假值。解决：kretprobe 加 `/@start[tid]/`。printf 检查输入即可确认：

```awk
printf("%d duration_ms=(%d-%d)/1000000\n", tid, nsecs, @start[tid]);
```

#### 展开成通用模式：双探针计时陷阱

这个 bug 值得泛化记住，因为它**寄生在全族计时工具里**（runqlat、biolatency、tcplife、execsnoop……书第 6-10 章会反复遇到）。时序：

```
                    工具启动
                       │
 ──────────────────────┼──────────────────────────────────► t
                       │
     [调用 A：入口在启动前]        [调用 B：完整经历入口+出口]
      kprobe 没赶上                 kprobe ✓ kretprobe ✓
      kretprobe ✓ ← 灾难在这         duration 正确
      @start[tid] 不存在 → 0
      nsecs - 0 ≈ 开机以来的纳秒数
```

三条判别线索（不重跑也能认出它）：

| 线索 | 指向 |
|------|------|
| 离群值量级 ≈ 系统 uptime（纳秒） | 入口未记录（本 bug），不是真实慢调用 |
| 离群值集中在工具刚启动的头几秒 | 同上——只有"存量"在飞调用会中招 |
| 离群值随时间持续出现且量级随机 | 多半是**真离群点**（page cache 冷读、锁竞争……）另查 |

修复模板（对照 5.5 的计时纪律）：

```awk
kprobe:fn     { @start[tid] = nsecs; }
kretprobe:fn  /@start[tid]/          // ← 三件套之一：出口侧过滤
{
    $ns = nsecs - @start[tid];
    delete(@start[tid]);              // ← 之二：用完即删（防 tid 复用串味）
    @ns = hist($ns);                  // ← 之三：先删后聚合，删漏的下次又被过滤挡
}
```

**tid 复用是第二层坑**：线程退出后 tid 会被新线程复用。若只过滤不 delete，老线程的残留 @start 会算到新线程头上——量级不对（两个活跃线程的时间差，通常是秒级而非 uptime 级），更隐蔽。所以 delete 不是洁癖，是正确性的一部分。

### 5.17.2 调试模式（-d）

- `-d` **不运行程序**，展示语法分析→LLVM IR 转换过程（主要面向 bpftrace 开发者）；
- 先打印**抽象语法树（AST）**：

```
# bpftrace -d -e 'k:vfs_read { @[pid] = count(); }'
Program
 k:vfs_read
  map: @
   builtin: pid
  call: count
```

- 再打印 **LLVM IR 汇编**（`target triple = "bpf-pc-linux"`、`@llvm.bpf.pseudo`、map_lookup/update 调用等）；
- `-dd` 打印优化前+优化后 IR。

### 5.17.3 详情模式（-v）

- 运行时打印**额外信息**：程序 ID、**BPF 字节码**（`(85) call bpf_get_current_pid_tgid` …）、验证器状态行（`from 9 to 12: safe`、`processed 22 insns, stack depth 16`）、挂载的探针；
- 程序 ID 可配合 **bpftool**（第 2 章）打印 BPF 内核状态；
- 与 -d 一样主要对核心开发者有用，普通用户无需关心字节码。

### -d vs -v 对照（应试重点）

| | `-d` / `-dd` | `-v` |
|--|--------------|------|
| 阶段 | **编译期**（dry run，不加载不运行） | **运行期**（真的加载并跑） |
| 看到什么 | AST → LLVM IR（-dd 含优化前后两份） | 程序 ID、BPF 字节码、验证器日志、挂载的探针 |
| 什么时候用 | "编译到底生成了什么"——验证宏/展开/优化怀疑 | "验证器为什么拒/挂了什么"——运行失败取证 |
| 对应流水线（5.16） | codegen 之前的检查 | bpf_load_prog 之后的环节 |

## HFT 关联

- 90% 的 bpftrace 排障 = printf + 检查过滤器；遇到验证器拒绝（如栈超 512B、访问非法指针）才上 -v 看字节码与 stack depth；
- "离群点 = 入口未记录"这一模式在所有双探针计时工具（runqlat、biolatency…）中通用，读这些工具源码时注意它们如何处理；
- 交易机延迟报告里的"尾部离群"出报告前先过一遍本节三线索：**离群点若是 uptime 级，是测量 bug 不是市场/系统事件**——把测量 bug 当系统事故复盘，是观测驱动团队最容易犯的低级错误之一。

## 陷阱

- ⚠️ 直方图离群点先查 `/@start[tid]/` 过滤器，再怀疑系统真有问题。
- ⚠️ -v 输出的 `processed N insns, stack depth N` 是验证器视角——stack depth 接近 512 时 str()/局部变量多就会失败。
- ⚠️ 过滤器修了离群点但还有"秒级怪值"——查 delete() 是否漏了（tid 复用串味，见上文第二层坑）。

<details>
<summary>自测题</summary>

1. vfs_read 计时出现巨大离群值的原因？
   <details><summary>答案</summary>kretprobe 对"入口未记录"（工具启动前已在执行）的调用也触发，nsecs-0 产生假值；需加 /@start[tid]/ 过滤。</details>

2. -d 与 -v 的区别？
   <details><summary>答案</summary>-d dry run 打印 AST 和 LLVM IR（不运行）；-v 运行时打印程序 ID、BPF 字节码、验证器状态。</details>

3. 加了 /@start[tid]/ 后直方图仍有秒级怪值（不是 uptime 级），下一怀疑对象？
   <details><summary>答案</summary>delete(@start[tid]) 漏写——tid 复用后新线程读到老线程的残留时间戳，量级是两线程活跃时间差（秒级）。</details>

4. 怎么快速判断离群点是"测量 bug"还是"真实慢事件"？
   <details><summary>答案</summary>量级对比：离群值 ≈ 系统 uptime（纳秒）→ 入口未记录的测量 bug；持续出现且量级随机 → 真离群点；集中在工具启动头几秒 → 存量在飞调用中招（也是测量 bug）。</details>
</details>
