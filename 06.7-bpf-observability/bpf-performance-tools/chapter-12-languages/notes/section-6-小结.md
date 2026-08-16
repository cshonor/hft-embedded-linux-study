# 6. 小结（12.6）

> 底本：《BPF之巅》第 12 章 编程语言，12.6 节（印刷 p619）

## 本章要点回顾

1. **分类决定路径**：编译型（C/C++/Go）→ 符号+帧指针+uprobe/kprobe 直达；JIT 型（Java/Node.js）→ 符号快照+采样+USDT；解释型（bash/Python）→ 解释器函数+结构体分析。
2. **编译型两条军规**：不 strip 符号、-fno-omit-frame-pointer。
3. **Java 三件套**：-XX:+PreserveFramePointer（栈）+ perf-map-agent/jmaps（符号，60 秒时效）+ profile/offcputime/stackcount（采样与事件栈）。
4. **高频探针是陷阱**：-XX:+ExtendedDTraceProbes 启用即 >10% 开销、使用可 >10x 减速——方法级跟踪只限实验室。
5. **Go 三坑**：uretprobe 不安全（栈可移动）、Plan9 栈传参（argN 不可用）、goroutine 迁移（tid 键不可靠）。
6. **稳定性阶梯**：USDT（稳定接口，优先）> 采样+符号 > uprobes（依赖版本实现）。
7. **方法论**：六步策略（问执行方式→看工具→查 USDT→写已知量样例→uprobe/kprobe）适用于任何新语言。

## 与其他章的衔接

- 栈回溯技术细节（LBR/DWARF/ORC/BTF）→ 第 2 章。
- profile/offcputime/stackcount 本体 → 第 6 章。
- 应用级案例（MySQL 等）→ 第 13 章。

## HFT 一句话

交易栈 C++ 主力：发布不 strip + 帧指针，即可对任意函数/栈/参数直接 BPF；Java 辅助服务用采样三件套；Go 网关记住"禁 uretprobe"。

<details>
<summary>自测题</summary>

1. 三类语言的插桩路径各是什么？
2. 稳定性阶梯排序及理由？
3. 为什么方法级 USDT 探针生产禁用？
</details>
