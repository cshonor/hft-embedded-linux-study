# Learning eBPF · 第 9 章：eBPF 与安全

> 底本：`../LEARNING-EBPF-BILINGUAL.pdf`。可观测工具只报告事件，安全工具要**区分正常与恶意并采取行动**。本章主线是一条演进链：seccomp → syscall 追踪（Falco）→ BPF LSM → Tetragon 内核函数挂载 + 同步阻断，每一步都在解决上一步的漏洞。

## 本章目标

1. 理解安全可观测性 = 策略（正常/异常判定）+ 上下文（事件发生时的完整信息）
2. 掌握 seccomp-bpf 的工作方式与局限，了解 eBPF 自动生成 seccomp profile 的方法
3. 理解 syscall 入口做安全的致命缺陷：**TOCTOU 竞态窗口**
4. 掌握 BPF LSM（参数已进内核内存后的权威检查点）与 Tetragon 的内核函数挂载思路
5. 理解检测型（异步通知）vs 防护型（`bpf_send_signal` 同步 SIGKILL）安全的区别

## 1. 安全可观测性 = 策略 + 上下文

- **策略（policy）**定义什么是预期行为：写 `/home/<user>/<file>` 正常，写 `/etc/passwd` 可疑
- 策略必须覆盖**错误路径**：磁盘满了触发的告警网络消息不寻常但不可疑——这是写策略最难的部分
- **上下文**决定调查能力：能不能回答"是否攻击、影响哪些组件、怎么发生的、谁干的"——能，才配叫"安全可观测性"而不只是日志
- 超出策略的事件 → 安全事件日志 → SIEM 平台 + 人工告警

## 2. 基于系统调用的安全（三代方案）

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

## 3. BPF LSM：权威检查点（5.7+）

- LSM（Linux Security Module）接口提供**数百个钩子**，每个都在"内核即将对内核数据结构操作之前"触发——此时参数已拷入内核内存，**不存在 TOCTOU**
- 钩子与 syscall 无一一映射，但任何安全敏感 syscall 都会触发一个或多个钩子
- `BPF_LSM` 程序类型让 eBPF 挂上这些钩子（第 7 章：返回非零 = **拒绝操作**）：

```c
SEC("lsm/path_chmod")
int BPF_PROG(path_chmod, const struct path *path, umode_t mode)
{
    bpf_printk("Change mode of file name %s\n", path->dentry->d_iname);
    return 0;   // 非零 → 拒绝本次 chmod
}
```

- 参数是内核数据结构（`path->dentry->d_iname` 直接是文件名）——策略判断**完全在内核内完成，高性能**
- 限制：需要 5.7+ 内核且开启 `CONFIG_BPF_LSM`（boot 参数 `lsm=bpf`），成书时多数发行版还没普及

## 4. Cilium Tetragon：挂内核内部函数

LSM 普及前的另一条路：把 eBPF 程序挂到**任意内核函数**（不限于稳定接口）。

- 依据：syscall/LSM 只是 3000 万行内核代码中极小的稳定部分；大量内部函数事实上多年未变；新内核普及需数年，不兼容有充足时间修复
- Tetragon 贡献者含内核开发者，凭内部知识挑出**安全且信息完备**的挂点
- K8s 自定义资源 **TracingPolicy** 声明式定义：挂点 + 条件 + 动作

```yaml
spec:
  kprobes:
  - call: "fd_install"        # 文件打开后在 fd 数组装入 file 指针（此时文件结构已填充完）
    matchArgs:
    - index: 1
      operator: "Prefix"
      values: ["/etc/"]       # 只关心 /etc/ 下的文件
```

- 挂 `fd_install` 而非 `open` syscall 的原因：它在**文件数据结构填充完之后**调用——和 LSM 同理，天然免疫 TOCTOU
- 内核内过滤：只把**超出策略**的事件报告用户态，而不是全量上报再筛

## 5. 防护型安全（preventative）

- 检测型模式：eBPF → 异步通知用户态 → 用户态处理——窗口期内数据可能已外泄、恶意代码可能已落盘
- **同步阻断**：`bpf_send_signal()`（5.3+）在 eBPF 程序内直接给违规进程发 **SIGKILL**——内核正在执行的动作被同步终止，来不及完成
- 实操建议：先跑**审计模式**（只告警不杀），确认策略无误报后再开 Sigkill——错误策略会杀掉正常应用
- 网络安全早已是防护模式（防火墙直接丢包；XDP offload 后恶意包根本到不了 CPU）；主机侧因为工具误报率高长期只能审计——eBPF 细粒度准确的控制正在把"可预防"从网络扩展到非网络事件

## 6. 坑点清单

1. **syscall 入口探针不能当安全工具用**——TOCTOU 可被"幻影攻击"绕过；观测用途没问题（第 7 章伏笔的完整答案）
2. **seccomp profile 覆盖错误路径**：压力/故障场景触发的 syscall 若不在 profile 里，应用会在最需要报警的时候死掉
3. `seccomp_unotify` 官方明确不可用于安全策略——别拿它做防护
4. **BPF LSM 需要 5.7+ 且启用** `lsm=bpf`；生产前先确认内核配置
5. 挂内核内部函数（Tetragon 式）没有稳定保证——升级内核要回归测试；选挂点必须选"数据结构已填充完"的位置
6. Sigkill 策略先审计后强制；从审计切到强制前复盘所有历史告警
7. seccomp profile 只能在进程启动时应用——对已运行进程要用 syscall 追踪类工具（Falco/Tetragon）

## 7. HFT 关联

- **交易进程 syscall 白名单**：交易进程只需要 40-70 个 syscall，用 eBPF 记录 + seccomp 收紧，缩小被入侵后的行动空间（供给面防御）
- **关键配置文件防护**：Tetragon 式策略盯 `/etc/`（交易所网关证书、密钥、hosts）的写入，Sigkill 同步阻断——比事后取证好
- **TOCTOU 教训泛化**：任何"检查用户态内存再让内核处理"的审计逻辑都有竞态；合规审计挂点要选内核侧数据结构
- **内核内过滤省带宽**：只上报违规事件，交易机上安全监控的常驻开销可忽略——不用为审计牺牲微秒

## 8. 自测题

1. 安全工具与可观测工具的本质区别是什么？策略为什么要考虑错误路径？
2. seccomp 严格模式允许哪四个 syscall？seccomp-bpf 的四种输出动作？
3. seccomp-bpf 的两大局限是什么？
4. eBPF 生成 seccomp profile 挂在哪个 tracepoint？为什么说每个进程加载独立程序是常见模式？
5. 画出 TOCTOU 竞态窗口。为什么 seccomp-bpf 反而不受影响？Sysmon for Linux 如何缓解、代价是什么？
6. BPF LSM 为什么天然免疫 TOCTOU？返回值语义是什么？需要哪个内核版本？
7. Tetragon 为什么选 `fd_install` 而不是 `open` syscall？
8. `bpf_send_signal` 实现的防护与"通知用户态再处理"有何本质不同？
9. 为什么网络安全工具普遍用防护模式而主机侧长期用审计模式？

## 9. 交叉引用

- 第 7 章 `07-程序类型.md`：LSM 程序类型返回码、syscall kprobe、raw tracepoint
- 第 8 章 `08-eBPF网络.md`：XDP 防火墙/DDoS、NetworkPolicy——网络侧防护模式
- 第 6 章 `06-验证器.md`：为什么 eBPF 程序不能随意解引用用户态指针
- 第 10 章 `10-eBPF编程.md`：gobpf 等 Go 库（本章 OCI hook 示例的用户态实现）
