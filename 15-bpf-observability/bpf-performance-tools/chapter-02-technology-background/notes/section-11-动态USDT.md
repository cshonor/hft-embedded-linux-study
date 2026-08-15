# 2.11 动态 USDT（给 JIT 语言加静态探针）

> 底本：《BPF之巅》第 2 章技术背景，2.11 节（印刷 p66–67）

## 解决什么问题

常规 USDT 需要预编译进 ELF 文件的 notes 段。但 Java/JVM 类语言**运行时解释或 JIT 编译**，JIT 出的代码没有 ELF、没有 notes 段 → 常规 USDT 加不进去。（JVM 自身的 C++ 部分倒是有很多内置 USDT 探针：GC、类加载等。）

## 动态 USDT 的三步方案

1. **预编译一个共享库**（C/C++），内含想要的 USDT 探针和 ELF notes 段——它与普通 USDT 探针一样可被插桩。
2. 运行时用 **dlopen(3)** 把该库加载进目标进程。
3. 在目标语言中通过合适的 API 封装，调用该库中的探针函数。

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

## 陷阱

- 动态创建的 so 映射地址每次运行不同，手工定位探针不现实——必须经 libstapsdt/BCC/bpftrace 走标准流程。
- 与信号量场景类似：`fire()` 的匿名函数里也别做重活，attach 期间它每事件执行一次。

## 自测

<details>
<summary>1. 为什么 JIT 代码无法直接用常规 USDT？</summary>

常规 USDT 需要 ELF 文件的 .note.stapsdt 段（编译期产物）；JIT 代码运行时生成，没有 ELF notes。
</details>

<details>
<summary>2. 动态 USDT 的核心思路是什么？</summary>

预编译带 USDT 探针的共享库 → dlopen 装入进程 → 目标语言 API 调用库内探针函数（libstapsdt 运行时生成并映射该库）。
</details>
