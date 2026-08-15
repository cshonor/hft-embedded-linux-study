# 3. BPF 工具：进程与命令执行（11.2.1–11.2.4）

> 底本：《BPF之巅》第 11 章 安全，11.2 节（印刷 p523–528）

表 11-1 安全工具总览（BT=bpftrace）：execsnoop（BCC/BT）、elfsnoop（本书 BT）、modsnoop（本书 BT）、bashreadline（BCC/BT）、shellsnoop（本书）、ttysnoop（BCC/本书）、opensnoop（BCC/BT）、eperm（本书 BT）、tcpconnect/tcpaccept（BCC/BT）、tcpreset（本书 BT）、capable（BCC/BT）、setuids（本书 BT）。

## 3.1 execsnoop —— 新进程执行（11.2.1）

```
PID   PPID  PCOMM   RET ARGS
7777  21086 ls      0   /bin/ls -F
7778  21086 a.out   0   /tmp/a.out
```

- 跟踪 `execve(2)`（fork/clone + execve 是新建进程的典型流程）。第 6 章已介绍，此处用于识别可疑进程（如 /tmp/a.out）。
- **局限**：缓冲区溢出攻击直接向现存进程注入指令，**不经 execve**——execsnoop 看不到。

## 3.2 elfsnoop —— ELF 二进制加载（11.2.2，本书 2019-02-25）

```
TIME        PID   INTERPRETER    FILE             MOUNT   INODE      RET
11:18:43    9022  /bin/ls        /bin/ls          /       29098068   0
11:18:45    9023  /tmp/ls        /tmp/ls          ...
```

- kprobe `load_elf_binary()`：内核深处**所有 ELF 执行必经**的函数。
- 输出 MOUNT + INODE 组合：攻击者可伪造同名二进制（甚至用控制字符伪装路径），但**骗不过挂载点+inode 唯一标识**。
- 源码要点：kprobe 存 arg0 到 `@arg0[tid]`，kretprobe 读 `struct linux_binprm *` 取 `interp/filename/f_path.mnt/i_ino/retval`。
- bpftrace 限制：printf 最多 7 个参数——打印更多信息需拆多个 printf。
- 开销可忽略（该函数调用频率极低）。

## 3.3 modsnoop —— 内核模块加载（11.2.3，本书 2019-03-14）

```
12:51:38 module_init: msr, by modprobe (PID 32574, user root, UID 0)
```

- kprobe `do_init_module()`，读 `struct module *` 的 name。
- 加载内核模块 = 系统执行代码的另一途径、后门工具的工作方式 → 安全跟踪目标。
- 另有 `module:module_load` 跟踪点（见单行程序）。

## 3.4 bashreadline —— 交互式命令记录（11.2.4）

```
TIME      PID    COMMAND
11:43:51  21086  echo hello book readers
11:44:22  21086  eccho hi        ← 失败命令也记录
11:44:33  21086  /tmp/ls
```

- uretprobe `/bin/bash:readline`，显示输入的任何命令（含 shell 内建与失败命令）。
- **盲区**：只跟踪 bash；攻击者可装自己的 shell（如 nanoshell）躲避。
- 发行版差异：某些 bash 用 libreadline 的 readline()——跟踪方法见 12.2.3。

## HFT 关联

- 三件套部署于跳板机/管理机：execsnoop（进程）+ bashreadline（命令）+ opensnoop（文件）= 管理面审计基线。
- elfsnoop 的 mount+inode 思想可直接用于交易程序完整性校验（防同名替换）。

<details>
<summary>自测题</summary>

1. execsnoop 的检测盲区是什么？
2. elfsnoop 为什么打印挂载点+inode？
3. bashreadline 的探针类型与函数？两种躲避方式？
</details>
