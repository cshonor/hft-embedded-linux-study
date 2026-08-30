# 3. 常见问题一：事件缺失与调用栈缺失（18.7–18.8）

> 底本：《BPF之巅》第 18 章，18.7–18.8 节（印刷 p764–767）

## 18.7 事件缺失

**症状**：事件可以被成功插桩，但是不触发，或者工具不输出结果。（如果事件根本不能插桩，见 18.10 节。）

**诊断法：用 perf(1) 交叉验证**——判断问题在 BPF 工具还是事件本身：

```bash
# perf stat -e block:block_rq_insert,block:block_rq_requeue -a
Performance counter stats for 'system wide':
        41  block:block_rq_insert
         0  block:block_rq_requeue
```

`block:block_rq_insert` 触发了 41 次、`requeue` 触发 0 次。若 BPF 工具同时跟踪了 insert 却没看到事件 → **BPF 工具的问题**；若 perf 和 BPF 都显示没有 → **事件本身没发生**。

kprobe 同理（perf 需要先创建再删除探针）：

```bash
# perf probe vfs_read
# perf stat -e probe:vfs_read -a
    3,029  probe:vfs_read      # 跟踪期间 vfs_read() 被调用了 3029 次
# perf probe --del probe:vfs_read
```

**两个常见原因**：

1. 软件修改后，之前被插桩的事件不再被调用
2. **从共享库位置跟踪库函数，但目标应用是静态编译的**——函数从应用程序二进制文件中调用

## 18.8 调用栈缺失

**症状**：打印的调用栈不完整或完全缺失（可能伴随 18.9 的符号问题，帧显示 `[unknown]`）。

示例：`trace -u t:syscalls:sys_enter_execve` 打印用户态栈：

```
PID    TID   COMM  FUNC                STACK
26853  26853 bash  sys_enter_execve    [unknown]
26854  26854 bash  sys_enter_execve    [unknown]
```

先用 perf(1) 交叉验证（`perf record` + `perf script`），perf 显示的类似不全调用栈暴露三个问题：

1. **调用栈不全**：跟踪 bash 调用新程序，经验上这有几个很深的帧，这里只有两帧。判断法：栈只有一两行、且没有以初始帧（如 `main` 或 `_start_thread`）结束 → 假设不完整
2. **最后一行 [unknown]**：perf 都不能解析符号——可能 bash 有符号问题，或 libc 的 `GI_execve()` 覆盖了帧指针中断遍历
3. **perf 能看到 libc `GI_execve()` 而 BCC 不能**：指向 BCC 跟踪的一个需要修复的问题（作者猜测 perf 用了 debuginfo，参见 bpftrace issue #646）

### 18.8.1 如何修复损坏的调用栈

不完整调用栈很常见，两个成因：

1. 观测工具使用**基于帧指针**的方法读取调用栈
2. 编译器性能优化没有预留寄存器（x86-64 的 **RBP**）给帧指针，而是复用为通用寄存器——工具读到的是数字/对象地址/字符串指针等任意值。幸运时解析不到符号打印 `[unknown]`；**不幸运时解析到不相干的符号，打印错误函数名的调用栈**，误导最终用户

**最简单的修复——恢复帧指针寄存器**：

| 软件类型 | 方法 |
|---|---|
| C/C++ 及其他 gcc/LLVM 编译软件 | 用 `-fno-omit-frame-pointer` 重新编译 |
| Java | 用 `-XX:+PreserveFramePointer` 运行 java(1) |

可能有性能损耗，但通常**观测到的损耗少于 1%**——可用调用栈定位性能问题的收益远大于此（第 12 章也有讨论）。

**另一种解决方法**：切换到不基于帧指针的堆栈遍历技术。perf(1) 支持 **DWARF**、**ORC**、**LBR**（最后分支记录）；但本书编写时，基于 DWARF 和 LBR 的堆栈遍历**在 BPF 中不可用**，ORC 不能用于用户态软件（详见 2.4 节）。

## HFT 关联

- 生产排障时"perf 交叉验证"是黄金法则：先确认事件真的发生，再怀疑 BPF 工具
- HFT 自研 C++ 策略/行情程序**构建管线应默认 `-fno-omit-frame-pointer`**（<1% 换全栈可见，本书反复强调）；Java 服务统一 JVM 参数 `-XX:+PreserveFramePointer`
- `[unknown]` 反而比"看似合理的错误符号名"安全——警惕后者静默误导排障方向

<details>
<summary>自测题</summary>

1. perf 交叉验证的两种结果分别指向什么结论？
2. 静态编译如何导致事件缺失？
3. 损坏调用栈的两个成因是什么？"不幸运"时会发生什么？
4. 修复帧指针的 C/C++ 和 Java 方法各是什么？ORC/DWARF/LBR 在 BPF 中的现状？

</details>
