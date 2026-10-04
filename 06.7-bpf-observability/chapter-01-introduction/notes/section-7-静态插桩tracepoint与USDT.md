# 1.7 静态插桩：tracepoint 和 USDT

> 底本：《BPF之巅》中文版 1.7 节（PDF p49–50）

## 动态插桩的两个软肋

1. **接口稳定性**：软件版本变更后，被插桩的函数可能被重命名或移除——升级内核/应用后，BPF 工具可能报"找不到函数"，也可能**静默无输出**（更危险）
2. **内联（inline）**：编译器优化会把函数内联掉，导致 kprobes/uprobes **无函数可插**。变通办法是函数偏移量跟踪（function offset），但它作为接口比函数入口**更不稳定**

补充第三个软肋（书里后文展开，先记结论）：**函数签名变化**——kprobe 挂上了，但参数布局变了，读出来的字段是垃圾。tracepoint 的 `format` 文件是自描述的，字段变化会显式暴露。

## 解法：静态插桩

把**稳定的事件名字**编码进软件代码、由开发者维护：

- **tracepoint**：内核的静态跟踪点
- **USDT**（user-level statically defined tracing）：用户态的静态定义跟踪

静态插桩的代价：插桩点增加开发者维护成本，所以**数量十分有限**。

## 机制速览：静态插桩为什么"稳定且近乎零开销"

tracepoint 的实现基础是 **static key（jump label）**：

```text
未启用:   代码里是一条无条件跳转（nop 5 字节 / 直接跳过探针体），开销≈0
启用后:   内核把该指令原地 patch 成跳到探针处理器的分支
```

- 每个静态跟踪点在 `/sys/kernel/tracing/events/<族>/<事件>/` 下有 **format 文件**——自描述字段名和偏移，BPF 程序按名取字段（`args->filename`），**不猜结构布局**
- 这也是 tracepoint 稳定性的实质：**字段名是开发者承诺的接口**，改字段名等于改 ABI，要过内核评审
- USDT 同理：`DTRACE_PROBE(provider, probe)` 宏在二进制的 note 段落登记探针位置与参数，`readelf -n` 可见；运行时 uprobe 挂到登记的地址上

## 选型策略（作者明确推荐）

> 先试静态（tracepoint / USDT），不够用再转动态（kprobes / uprobes）。

稳定、不受内联影响；但覆盖面窄，动态插桩是"任何函数都能挂"的兜底。

| | tracepoint/USDT | kprobe/uprobe |
|---|---|---|
| 接口稳定性 | 字段名是承诺的 ABI | 函数名/签名随时可变 |
| 内联影响 | 无 | 内联后无函数可挂 |
| 覆盖面 | 数量有限（开发者埋多少有多少） | 任意函数 |
| 开销 | static key，未启用≈0 | 断点陷入 |
| 自描述 | format 文件/note 段 | 靠开发者自己查内核源码签名 |

## bpftrace 探针写法（表 1-3）

```text
tracepoint:syscalls:sys_enter_open        对 open(2) 系统调用插桩
usdt:/usr/sbin/mysqld:mysql:query__start  mysqld 中的 query__start 探针
```

> 脚注：系统调用跟踪点需要内核编译时开启 `CONFIG_FTRACE_SYSCALLS`。

实用检查命令——排障第一步先确认探针存在：

```bash
bpftrace -l 'tracepoint:syscalls:*open*'     # 列出 syscall 跟踪点
cat /sys/kernel/tracing/events/syscalls/sys_enter_openat/format   # 看字段定义
bpftrace -l 'usdt:/usr/sbin/mysqld'          # 列出二进制里的 USDT 探针（需 debug info）
```

---

### HFT 关联

- 交易机上长期挂载的观测脚本应**优先绑 tracepoint**（内核小版本升级不破），临时排障才用 kprobe——运维友好性差异巨大
- USDT 思路可移植到自家策略引擎：在**策略框架代码里埋稳定的 USDT 探针**（order__submit、signal__generated），版本迭代中观测脚本不用改——这是把"插桩稳定性"内建进自己软件的正确方式
- "静默无输出"是排障大坑：脚本挂上一个已不存在的 kprobe 目标时，先确认探针真的 attach 上了（bpftrace 会打印 Attaching N probes... 的数量）
- format 文件自描述字段 = 写 BPF 程序前**必读的接口文档**；kprobe 没有等价物（要读内核源码确认参数），这是排障时间成本的隐性差异

<details>
<summary>📝 自测题（点击展开）</summary>

1. **静态插桩和动态插桩各自的优缺点与推荐使用顺序？**

   <details><summary>参考答案</summary>

   静态（tracepoint/USDT）：接口稳定、不受内联影响，但插桩点少、依赖开发者维护。动态（kprobes/uprobes）：任意函数可插、零成本启用，但内核/应用升级后可能失效甚至静默无输出，且被内联的函数挂不上。推荐顺序：先静态、不够再动态。

   </details>

2. **为什么函数偏移量跟踪比函数入口跟踪更不稳定？**

   <details><summary>参考答案</summary>

   函数入口有符号名做锚点；偏移量是相对函数内某条指令的硬编码位置，任何一次重新编译（哪怕函数逻辑没变，只是指令布局变化）都可能让偏移失效，且失效后探针挂到错误指令上，行为不可预测。

   </details>

3. **tracepoint 的 format 文件解决了什么问题？kprobe 为什么没有等价物？**

   <details><summary>参考答案</summary>

   format 自描述事件字段（名字+偏移+类型），BPF 程序按名取数，不依赖内核版本的结构布局；字段变更对使用者可见。kprobe 挂的是任意函数，内核不承诺其签名——参数布局只能靠使用者读内核源码确认，版本升级后无任何提示，属于"无契约接口"。

   </details>

4. **如何提前发现"探针静默失效"？**

   <details><summary>参考答案</summary>

   部署时验证而非运行时等待：bpftrace 启动时打印 "Attaching N probes..."，脚本应断言 N 符合预期（BCC 可在 Python 层检查 attach 结果）；CI 里跑一次工具 smoke 测试；对关键常驻脚本加心跳输出（interval 块定期打印），输出停止 = 探针失效或事件流断流的告警信号。

   </details>

</details>
