# 9. 小结（11.4）

> 底本：《BPF之巅》第 11 章 安全，11.4 节（印刷 p544）

## 本章要点回顾

1. **BPF 用于安全的三大类能力**：实时取证嗅探（shellsnoop/ttysnoop）、权限调试（capable/setuids/eperm）、白名单构建（elfsnoop 的 mount+inode、capable 的能力清单）。
2. **相对 LKM 方案**：验证器保证安全、开销约为 auditd 的 1/6。
3. **事件洪水风险**：BPF 缓冲/映射有限可被攻击者淹没 → 必须记录溢出，用每 CPU 计数器保证计数不丢。
4. **策略执行已超出观测**：seccomp/Cilium/bpfilter/Landlock/KRSI + bpf_send_signal（5.3）实现检测即阻断。
5. **12 个工具两层复用**：execsnoop/opensnoop/tcpconnect/tcpaccept 来自性能章节（视角换为"谁干了什么"），elfsnoop/modsnoop/shellsnoop/ttysnoop/eperm/tcpreset/capable/setuids 为本书新开发。
6. **零日响应方法论**：bpftrace 数分钟内写出特征检测（Docker renameat2 案例），未来漏洞披露可附带检测单行。

## 与其他章的衔接

- 网络连接细节 → 第 10 章；文件使用 → 第 8 章；进程执行 → 第 6 章。
- 容器内跑 BPF 的特权问题 → 第 15 章。
- libreadline 跟踪差异 → 12.2.3。

## HFT 一句话

交易基础设施的安全观测三板斧：**capable 建最小权限白名单、tcpconnect 守出口连接清单、bpftrace 单行做零日应急**——全部低开销可常驻管理面。

<details>
<summary>自测题</summary>

1. 本章哪些工具复用自性能章节？哪些是本书新开发？
2. BPF 安全方案必须记录什么以满足不可反悔要求？
</details>
