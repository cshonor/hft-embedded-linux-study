# 5. BPF 工具：文件与权限拒绝（11.2.7–11.2.8）

> 底本：《BPF之巅》第 11 章 安全，11.2 节（印刷 p532–533）

## 5.1 opensnoop —— 文件打开审计（11.2.7）

```
PID    COMM      FD   ERR PATH
12748  opensnoop -1   2   /usr/lib/python2.7/encodings/ascii.x86_64-linux-gnu.so
12748  opensnoop 18   0   /usr/lib/python2.7/encodings/ascii.py
1222   polkitd   11   0   /etc/passwd
1222   polkitd   11   0   /proc/11881/status
```

- 跟踪 open(2) 族系统调用（第 8 章已介绍），安全视角用途：
  - **理解恶意软件行为**（它打开/读写了哪些文件）；
  - 监控文件使用（如 polkitd 读 /etc/passwd + /proc/<pid>/status 的组合是否正常）。
- 输出含 FD/ERR：失败的打开（ERR=2 ENOENT / 13 EACCES）同样记录——侦察行为特征。

## 5.2 eperm —— 权限拒绝统计（11.2.8，本书 2019-02-25）

```
@EACCES[systemd-logind, sys_setsockopt]: 1
@EPERM [cat, sys_openat]: 1
@EPERM [gmain, sys_inotify_add_watch]: 6
```

- 统计因 **EPERM（operation not permitted）/ EACCES（permission denied）** 失败的系统调用，按 [comm, syscall] 分组。
- 实现：跟踪 `raw_syscalls:sys_exit` 跟踪点，按 `args->ret == -1 / -13` 分组。
- **syscall 名转换技巧**：raw tracepoint 只给调用号——eperm 不用查询表（如 BCC syscount 那样），而是**读内核系统调用表 `sys_call_table`**：`ksym(*(kaddr("sys_call_table") + args->id * 8))` 把处理函数地址转成符号名。
- 开销警告：sys_exit 对**所有**系统调用触发，高 I/O 系统上开销明显——先在实验室测试。

## 5.3 配合模式

opensnoop（细节）与 eperm（汇总）互补：eperm 发现异常组合 → opensnoop -x 过滤失败打开看具体路径。

## 5.4 机制：sys_call_table 技巧为什么值得学

eperm 的调用号→名字转换不走用户态查询表，而是直接读内核符号：

```text
kaddr("sys_call_table")              系统调用表基地址（符号解析）
        + args->id * 8               偏移 = 调用号 × 8（64 位指针）
        → *(kaddr)                   取该表项 = 处理函数地址
        → ksym(...)                  地址 → 函数符号名（如 sys_openat）
```

- 精髓：**内核自己维护的映射就是最权威的查询表**——不用在用户态复制一份（复制就要随内核版本维护）
- 代价：耦合内核布局（表项是地址不是名字，CONFIG_RETHUNK/CFI 内核上 ksym 解析可能给出别名）；跨版本工具要测
- 这个"读内核现成数据结构代替自建映射"的手法是 bpftrace 高级单行的常见套路（对照 elfsnoop 读 `linux_binprm`、capable 读 `cap_capable` 参数）

## HFT 关联

- 权限收紧回归测试：给交易进程降权（去 CAP_SYS_ADMIN 等）后跑 eperm，逐条确认无业务受阻。
- EACCES/EPERM 洪水也是**提权尝试**的信号（攻击者试探 sudoers/配置错误）。

<details>
<summary>自测题</summary>

1. eperm 用什么跟踪点？开销为什么可能明显？
2. eperm 如何把系统调用号转成名字（不用查询表）？
3. opensnoop 在安全分析中的两个用途？

<details><summary>参考答案</summary>

1. raw_syscalls:sys_exit——对**所有**系统调用的退出都触发，事件率 = 全机 syscall 率（高 I/O 机器每秒数十万）。逐事件过滤 ret 值的固定成本乘上这个频率就明显了，所以书里要求先实验室测。
2. `ksym(*(kaddr("sys_call_table") + args->id * 8))`：把调用号当索引算出表项偏移，读出处理函数地址，再符号化。内核的系统调用表本身就是权威映射，无需用户态复制。
3. ①理解恶意软件行为（它打开/读写哪些文件——行为画像）；②监控敏感文件使用（如 polkitd 读 /etc/passwd + /proc/pid/status 的组合是否正常）。失败打开（ERR 列）还提供侦察行为特征——攻击者在探测文件存在性。
</details>
</details>
