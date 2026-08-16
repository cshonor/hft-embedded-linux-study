# 验证器日志

- `bpftool prog load` 失败时日志打到 stderr；libbpf 程序用 `libbpf_set_print()` 接收；也可以强制在**成功**时也输出日志（传一个日志缓冲区给加载调用）
- 日志尾部的工作量摘要：

```
processed 61 insns (limit 1000000) max_states_per_insn 0 total_states 4 peak_states 4 mark_read 3
```

`total_states` 是累计存储状态数，`peak_states` 是峰值；有剪枝时 peak < total。

- 日志主体 = 指令 + （`-g` 编译时的）C 源码行 + 寄存器状态快照：

```
0: (bf) r6 = r1
; data.counter = c;
1: (18) r1 = 0xffff800008178000
3: (61) r2 = *(u32 *)(r1 +0)
 R1_w=map_value(id=0,off=0,ks=4,vs=16,imm=0) R6_w=ctx(id=0,off=0,imm=0) R10=fp0
; c++;
5: (07) r3 += 1
 R3_w=inv(id=0,umin_value=1,umax_value=4294967296,var_off=(0x0; 0x1ffffffff)) ...
```

**读日志的关键心法**（作者：先有 eBPF 虚拟机的心智模型，这些提示才有用）：

| 寄存器 | 约定 |
|---|---|
| R1 | 程序入口 = ctx；调 helper 时 = 第 1 个参数（R1-R5 传参） |
| R0 | helper 返回值；也是程序返回值 |
| R6-R9 | helper 调用**不会破坏**的寄存器——所以开头常见 `r6 = r1` 把 ctx 存进 R6，之后随便调 helper 都不丢 |
| R10 | 栈帧指针（fp），程序不可修改 |

- 值域示例：`c++` 之后 R3 的 `umin_value=1`、`umax_value=0xFFFFFFFF`——验证器就是这样用范围信息做越界推理和剪枝的
- 可视化控制流：`bpftool prog dump xlated name kprobe_exec visual > out.dot && dot -Tpng out.dot > out.png`
