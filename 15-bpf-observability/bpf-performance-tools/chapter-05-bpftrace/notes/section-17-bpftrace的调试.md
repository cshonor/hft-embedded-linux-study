# 5.17 bpftrace 的调试

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.17 节（印刷 p186–189）

## 内容详解

定位问题先看 **第 18 章**（事件丢失、调用栈残缺、符号不完整等常见问题）。

**与 BCC 的本质差异**：bpftrace 由一组稳定的、设计上可安全共存的功能集组成，倾向失败时弹**用户友好错误消息**，一般不需进一步调试；BCC 允许 C/Python 自由发挥，能力更广但不保证协同工作，更常需要调试模式。

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

## HFT 关联

- 90% 的 bpftrace 排障 = printf + 检查过滤器；遇到验证器拒绝（如栈超 512B、访问非法指针）才上 -v 看字节码与 stack depth；
- "离群点 = 入口未记录"这一模式在所有双探针计时工具（runqlat、biolatency…）中通用，读这些工具源码时注意它们如何处理。

## 陷阱

- ⚠️ 直方图离群点先查 `/@start[tid]/` 过滤器，再怀疑系统真有问题。
- ⚠️ -v 输出的 `processed N insns, stack depth N` 是验证器视角——stack depth 接近 512 时 str()/局部变量多就会失败。

<details>
<summary>自测题</summary>

1. vfs_read 计时出现巨大离群值的原因？
   <details><summary>答案</summary>kretprobe 对"入口未记录"（工具启动前已在执行）的调用也触发，nsecs-0 产生假值；需加 /@start[tid]/ 过滤。</details>

2. -d 与 -v 的区别？
   <details><summary>答案</summary>-d dry run 打印 AST 和 LLVM IR（不运行）；-v 运行时打印程序 ID、BPF 字节码、验证器状态。</details>
</details>
