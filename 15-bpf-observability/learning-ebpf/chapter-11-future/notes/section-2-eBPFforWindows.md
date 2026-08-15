# eBPF for Windows

- 成书时（2022 底）已有可运行演示：Cilium L4 负载均衡、eBPF 连接跟踪跑在 Windows 上
- 为什么可行：eBPF 本质是**内核中的虚拟机执行字节码**——VM 也可以在 Windows 里实现；网络包结构与协议栈处理在所有 OS 上是相通的

### 架构与许可证约束（本节最有意思的工程决策）

| 组件 | 选择 | 原因 |
|---|---|---|
| 工具链 | 复用 libbpf + Clang 的 eBPF 字节码支持 | 宽松许可，生态现成 |
| 验证器 | **PREVAIL**（非 Linux 内核验证器） | Linux 内核验证器是 GPL，Windows 闭源不能抄 |
| JIT | **uBPF** JIT | 宽松许可 |
| 验证/JIT 位置 | **用户态 Secure 环境**（非内核） | 内核里的 uBPF 解释器仅用于 debug 构建 |

- 别指望所有 Linux eBPF 程序都能跑在 Windows 上——但这与跨内核版本的 CO-RE 挑战本质相同：数据结构差异要程序员优雅处理
