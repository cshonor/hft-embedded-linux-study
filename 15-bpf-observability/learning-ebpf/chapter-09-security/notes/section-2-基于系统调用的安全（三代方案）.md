# 基于系统调用的安全（三代方案）

### 2.1 seccomp / seccomp-bpf（第一代：进程级沙箱）

- **严格模式**：只允许 `read/write/_exit/sigreturn` 四个 syscall
- **seccomp-bpf**（容器世界接触的版本）：用经典 BPF 指令做过滤器，每次 syscall 触发，可看**值参数**，输出四种动作：
  1. 放行
  2. 返回错误码给用户态
  3. 杀死线程
  4. `seccomp-unotify` 通知用户态（5.0+）
- **两大局限**：BPF 代码**不能解引用参数指针**（只看得到指针值，看不到文件名）；profile **必须在进程启动时应用**，运行中不能改
- Docker 默认 profile 是通用的：允许几乎所有 syscall，只禁 `reboot()` 这类 universally-inappropriate 的。Aqua 统计：容器应用实际只用 **40-70 个** syscall——收紧空间巨大

### 2.2 eBPF 自动生成 seccomp profile

开发者说不清自己程序用了哪些 syscall（高级语言抽象所致），所以要自动记录：

- 思路：eBPF 程序挂 `raw_syscalls:sys_enter` tracepoint，维护一个 map 记录见过的 syscall 号，跑一段时间后生成 JSON profile（K8s/OCI 运行时格式）
- 工具：**Inspektor Gadget** 的 seccomp profiler、Red Hat 的 **OCI runtime hook**
- 坑：记录期没覆盖到的**错误路径** syscall 会被 profile 挡掉，出错时行为异常且难排查——profile 难以人工审查
- 书中代码（OCI hook，Go + gobpf）：`$PARENT_PID` 字符串替换注入 pid，再 `m.AttachTracepoint("raw_syscalls:sys_enter", enterTrace)`——**每个被观测进程加载一份独立 eBPF 程序**的常见模式

### 2.3 syscall 追踪类安全工具（第二代：Falco）

- **Falco**（CNCF）：规则定义安全相关事件，违规时告警；默认内核模块实现，也有 eBPF 版
- 挂点：`raw_syscalls/sys_enter`、`sys_exit`（还有 page fault 等）
- 对比 seccomp 的优势：**动态加载、可作用于已运行的进程、可随时改规则**——不用重启应用

### 2.4 TOCTOU：syscall 入口做安全的死穴

```
用户态传指针 ──→ eBPF 程序在 syscall 入口检查参数（读到文件名 "/tmp/a"）
                     │
                     │  ← 竞态窗口：攻击者（另一线程）改写该内存
                     ▼
              内核 copy_from_user 把数据拷进内核（实际拿到 "/etc/shadow"）
```

- eBPF 检查的数据 ≠ 内核实际使用的数据 → **绕过检测**（DEFCON 29 "Phantom Attack" 演示，直接打 Falco）
- seccomp-bpf 反而没这个问题：它根本不许解引用指针（也就什么都看不到）
- `seccomp_unotify` 同样中招：man page 明确写"**不能用于实现安全策略**"
- **Sysmon for Linux** 的缓解：同时挂入口+出口，调用完成后查内核数据结构（如从 fd 反查文件对象）拿准确视图——但只能**记录**，不能**阻止**（syscall 已执行完）
