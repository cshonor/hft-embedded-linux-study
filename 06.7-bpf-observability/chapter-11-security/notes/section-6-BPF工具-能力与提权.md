# 6. BPF 工具：能力与提权（11.2.11–11.2.12）

> 底本：《BPF之巅》第 11 章 安全，11.2 节（印刷 p536–542）

## 6.1 capable —— 安全能力检查（11.2.11）

```
TIME      UID   PID    COMM   CAP  NAME             AUDIT
22:52:11  20007 21     ...    21   CAP_SYS_ADMIN    21
22:52:12  1000  20108  ssh    7    CAP_SETUID       ...
          sshd 检查 CAP_SETGID/CAP_SETUID/CAP_SYS_CHROOT ...
```

- **用途：构建应用所需能力白名单**——记录程序实际用到的 capabilities，只授予这些、阻止其他，实现最小权限。
- kprobe `cap_capable()`（决定当前任务是否具有给定能力的内核函数）；调用频率低 → 开销可忽略。
- 列：CAP（能力号）、NAME（名称，见 capabilities(7)）、AUDIT（该检查是否写审计日志）。
- 选项：-V 含非审计检查（默认排除）；-P PID 仅该进程；**-K 内核栈 / -U 用户栈**。
- 栈示例解读：bash 读 history 触发 open→capable(CAP_DAC_READ_SEARCH)，内核栈从 `cap_capable` → `openat` → `entry_SYSCALL_64`，用户栈 `open` → `read_history` → `main`——一眼看清**谁在什么路径上要什么权限**。
- bpftrace 版：`@cap[0..37]` 哈希表硬编码能力号→名（CAP_CHOWN … CAP_AUDIT_READ），内核新增能力需手动更新表。

## 6.2 setuids —— 提权系统调用（11.2.12，bpftrace 2019-02-26）

```
TIME      PID    COMM  UID   SYSCALL   ARGS(RET)
23:39:18  23436  sudo  1000  setresuid ruid=-1 euid=0 suid=-1 (0)
23:39:18  23436  sudo  1000  setuid    uid=0 (0)
```

- 跟踪 `setuid(2)`、`setresuid(2)`、`setfsuid(2)` —— 权限提升族系统调用。
- 典型输出：sudo 从 UID 1000 → 0 的完整序列；sshd 登录切 UID 同样可见（login/su/sshd 全覆盖）。
- 实现模式（教科书级入口/出口配对）：
  - sys_enter_* 跟踪点把 uid/参数存 `@uid[tid]`、`@setuid[tid]`、`@seen[tid]`；
  - sys_exit_* 跟踪点读映射打印（含 args->ret），再 delete 各键。
- (RET) 语义：setuid/setresuid 显示成败；**setfsuid 的返回值是之前的 UID**（历史 API 设计）。
- 调用频率低 → 开销可忽略。

## HFT 关联

- 交易/行情进程容器化时的权限最小化流程：跑 capable -KU 采样 → 生成白名单 → docker/k8s `cap_drop: [ALL]` + `cap_add: [...]`。
- setuids 部署在管理面：任何 UID→0 切换（计划外）立即告警。

<details>
<summary>自测题</summary>

1. capable 的白名单工作流是什么？跟踪哪个函数？
2. -V 选项处理什么？为什么默认排除？
3. setuids 的入口/出口配对如何实现？
4. setfsuid 的返回值语义与其他两个有何不同？

<details><summary>参考答案</summary>

1. kprobe cap_capable()（内核能力判定函数，所有 capable() 检查的汇聚点）。工作流：采样期跑 capable（可加 -K/-U 抓栈看调用路径）→ 汇总程序实际用到的能力集合 → 只授予这些（cap_drop ALL + cap_add 白名单），其余拒绝——最小权限从"猜"变成"测"。
2. -V 含非审计检查（audit=0 的 capable 调用，内核热路径里大量存在、只为快速判权不写审计日志）。默认排除是因为它们噪音多、安全分析价值低——写审计日志的检查才是"值得记录的权限事件"。
3. sys_enter_* 跟踪点把 uid 和参数存入以 tid 为键的 map（@uid[tid] 等），sys_exit_* 读出打印（含返回值），然后 delete 各键——**入口存参数、出口拿结果**的标准双探针模式（与 vfs_read 计时同构，见 5.17 的双探针陷阱）。
4. setuid/setresuid 返回 0=成功；**setfsuid 返回的是"之前的 fsuid"**（历史 API：它原本就没有错误返回，成败要用回读 fsuid 判断）——所以 setuids 输出里 setfsuid 行的 (RET) 不是成败标志。
</details>
</details>
