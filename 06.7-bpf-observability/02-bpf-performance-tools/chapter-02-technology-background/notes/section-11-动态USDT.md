# 2.11 动态 USDT（给 JIT 语言加静态探针）

> 底本：《BPF之巅》第 2 章技术背景，2.11 节（印刷 p66–67）

## 解决什么问题

常规 USDT 需要预编译进 ELF 文件的 notes 段。但 Java/JVM 类语言**运行时解释或 JIT 编译**，JIT 出的代码没有 ELF、没有 notes 段 → 常规 USDT 加不进去。（JVM 自身的 C++ 部分倒是有很多内置 USDT 探针：GC、类加载等。）

问题拆开看是两层：

1. **解释/JIT 代码本身**（如 Python 字节码循环里的一段逻辑）：它不经过链接器，永远不会有 ELF notes——动态 USDT 也救不了，这类只能靠语言级探针机制（如 Python 的 USDT 封装）或 uprobe 挂解释器内部
2. **语言代码想触发的探针点**（"策略脚本执行到这一行时通知我"）：这是动态 USDT 的用武之地——探针不在 JIT 代码里，而在**一个真正的共享库**里，语言代码只是"调用"它

## 动态 USDT 的三步方案

1. **预编译一个共享库**（C/C++），内含想要的 USDT 探针和 ELF notes 段——它与普通 USDT 探针一样可被插桩。
2. 运行时用 **dlopen(3)** 把该库加载进目标进程。
3. 在目标语言中通过合适的 API 封装，调用该库中的探针函数。

架构示意：

```text
┌────────── 目标进程（Node.js / Python / Ruby）──────────┐
│  语言代码:  probe1.fire(() => [reqStr])                  │
│               │ 仅当探针被 attach 时才执行回调            │
│               ▼                                          │
│  libstapsdt 动态生成/加载的小 .so                         │
│  ┌────────────────────────────────┐                      │
│  │ ELF .note.stapsdt + nop 探针    │ ← 外部跟踪器按普通    │
│  └────────────────────────────────┘   USDT 流程 attach    │
└──────────────────────────────────────────────────────────┘
```

**关键洞察**：动态 USDT 没有发明新机制——它只是把"探针所在的二进制"从"编译期链接的主程序"换成了"运行时 dlopen 的辅助库"，之后的一切（notes 段、nop→int3、attach 流程）复用常规 USDT 的全部基础设施。

## libstapsdt

Matheus Marchini 为 Node.js 和 Python 实现了 libstapsdt 库（其他语言可封装它，如 Dale Hamel 的 Ruby C 扩展）。libstapsdt 在**运行时自动创建包含 USDT 探针与 ELF notes 的共享库，并映射到运行中程序的地址空间**。

Node.js 示例：

```javascript
const USDT = require("usdt");
const provider = new USDT.USDTProvider("nodeProvider");
provider.enable();
// ...
probe1.fire(function () { return [currentRequestString]; });
```

`probe1.fire()` 只有在外部跟踪器插桩时才执行匿名函数——参数处理只发生在探针启用时，未启用直接跳过，无 CPU 开销。

（书中注：新的 libusdt 库开发中，接口可能变化。）

## HFT 关联

- HFT 大量使用 Python（策略研究/风控脚本）与 JIT 语言；动态 USDT 让这些代码也能拥有零开销静态探针——平时零成本，需要时 attach。
- `fire()` 回调惰性求值的模式值得记住：**任何自研观测点都应设计成"未启用时零成本"**。
- 更现代的替代路径要认识到：对 JVM 系，热点自带探针 + JVM 内置 USDT（GC/JIT 事件）往往够用，动态 USDT 是长尾补充；对 Python，语言层开销常在解释器侧，探针点选在 FFI/关键库边界比选在纯 Python 循环里有效。
- 交易系统的正确埋点层级：C++ 核心路径常规 USDT（编译期）、脚本层动态 USDT（运行时）、第三方黑盒 uprobe（应急）——三层各管一段。

## 陷阱

- 动态创建的 so 映射地址每次运行不同，手工定位探针不现实——必须经 libstapsdt/BCC/bpftrace 走标准流程。
- 与信号量场景类似：`fire()` 的匿名函数里也别做重活，attach 期间它每事件执行一次。
- dlopen 的库受 ASLR 影响，每次进程启动地址都变——观测脚本要按"库路径+探针名"定位（bpftrace usdt: 支持按 PID + 路径），别缓存上次会话的地址。

## 自测

<details>
<summary>1. 为什么 JIT 代码无法直接用常规 USDT？</summary>

常规 USDT 需要 ELF 文件的 .note.stapsdt 段（编译期产物）；JIT 代码运行时生成，没有 ELF notes。
</details>

<details>
<summary>2. 动态 USDT 的核心思路是什么？它发明了新机制吗？</summary>

预编译带 USDT 探针的共享库 → dlopen 装入进程 → 目标语言 API 调用库内探针函数（libstapsdt 运行时生成并映射该库）。没有发明新机制——只是把探针宿主从主程序换成运行时加载的辅助库，attach 流程完全复用常规 USDT 基础设施。
</details>

<details>
<summary>3. `fire()` 的惰性求值保证了什么？这个模式如何泛化？</summary>

保证未启用时零成本（回调不执行、参数不构造），启用时才付参数构造成本。泛化：一切自研观测点的接口设计都应是"检查启用状态 → 短路或执行"，把昂贵的部分放在检查之后。
</details>

<details>
<summary>4. 解释型语言里"探针埋在纯语言循环内"和"埋在 FFI/库边界"哪个更有效？为什么？</summary>

FFI/库边界。纯解释循环里的探针每次经过都吃解释器开销（无论探针自身多轻，跨语言调用的桥接成本固定存在）；库边界上事件频率低一个量级、且探针落在编译代码里（nop 开销真正为零），信噪比更好。
</details>
