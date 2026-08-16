# 第一个 Hello World（BCC 版）

```python
#!/usr/bin/python
from bcc import BPF
program = r"""
int hello(void *ctx) {
    bpf_trace_printk("Hello World!");
    return 0;
}
"""
b = BPF(text=program)                          # BCC 现场编译 C 字符串并加载进内核
syscall = b.get_syscall_fnname("execve")       # execve 的内核实现函数名随架构不同（x86: __x64_sys_execve）
b.attach_kprobe(event=syscall, fn_name="hello") # 挂 kprobe
b.trace_print()                                 # 无限循环读 trace
```

**分层理解：**
- eBPF 程序是 C，由 BCC 在运行时编译（下一章手工做这一步）
- `bpf_trace_printk()` 是 **helper 函数**——eBPF 程序不能调用任意内核函数，只能调用内核白名单里的 helper（区分 eBPF 与 classic BPF 的特性之一）
- 输出固定写到 `/sys/kernel/debug/tracing/trace_pipe`——**全机唯一**，多程序混写、只支持字符串、无结构化 → 只配调试用

**权限要点：**
- root 最简单；"Operation not permitted" 第一个怀疑非特权
- `CAP_BPF`（5.8+）只是基础：加载跟踪程序还需 `CAP_PERFMON`，加载网络程序还需 `CAP_NET_ADMIN`

**行为验证**：程序加载前就在跑的进程调用 execve 也触发——动态生效、零重启。
