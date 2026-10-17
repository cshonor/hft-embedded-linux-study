# 条款 50：了解 new 和 delete 的合理替换时机

## 本节讲什么

**Understand when it makes sense to replace new and delete.** 重载 operator new/delete
的成本不低（正确性陷阱多，→ item51/52）——先想清楚**为什么换**。
Meyers 给出五个正当理由，按价值排序：**检测错误 > 统计埋点 > 性能 >
对齐 > 定制行为**。前两个几乎零风险该常备，性能理由要先有 profile 证据。

← 上一条 [item49 类内 new/delete](./item49-了解class-specific的new-delete.md)；
下一条 [item51 new/delete 的常规](./item51-编写new和delete时需固守常规.md)。

---

## 1. 五个正当理由（按价值排序）

### ① 检测使用错误（价值最高，风险最低）

```cpp
void* operator new(std::size_t n) {
    void* p = std::malloc(n + GUARD_SIZE);
    write_guard_bytes(p, n);        // 首尾写魔数——delete 时校验越界写
    log_allocation(p, n);           // 记日志：delete 时配对检查泄漏
    return p;
}
```

能抓：越界写（guard bytes）、泄漏（分配/释放对账）、重复释放（状态位）、
new/delete 与 malloc/free 混用——**调试期神器**（ASan 的手工低配版；
有 ASan 就用 ASan，→ 03.6 ch3，这招的价值在老代码库/特殊平台）。

### ② 统计与埋点

全局重载记录：分配总量/峰值、尺寸分布、调用点栈——
回答"**谁在热路径偷偷分配**"（→ 19.1 HFT 的埋点用法）。
配合 `operator new(size_t, const char* file, int line)` 重载 + 宏
（`#define new new(__FILE__, __LINE__)`）可记录每处的调用点
（慎用宏版 new——与 placement new 语法冲突，工程上多用栈回溯替代）。

### ③ 性能（必须有 profile 证据）

默认分配器是**通用**设计——你的分配模式（固定尺寸、单线程、批量释放）可能
用专用池快一个量级。**但**：先测量证明 malloc 真的是瓶颈
（`perf record` 里 malloc/free 占比，→ 06.6.5 ch07），
再池化（→ item49 类内重载优先于全局重载——影响面小）。
LD_PRELOAD 换 jemalloc/tcmalloc 通常是**比手写全局重载更好的第一步**。

### ④ 对齐需求

`alignas(64)` 的类型在 C++17 前**不保证** new 返回对齐内存
（默认只保证 max_align_t 通常 16B）——SSE/AVX/缓存行对齐的类型
要重载 new 调 `aligned_alloc`/`posix_memalign`。
**C++17 起语言自带**：`new (std::align_val_t(64)) T`——这条理由在现代代码里
已经半退役，知道历史才能读懂老代码。

### ⑤ 定制行为（集群/共享内存/NUMA）

共享内存段上的分配（`shm_open` 区域内的池）、NUMA 节点本地分配、
持久内存（pmem）——这些场景的"new"本来就不是默认语义，重载是必然。

## 2. 什么**不是**好理由

- "我觉得 malloc 慢"（没 profile）——先测（06.6.5 的方法论：USE/下钻）
- "想让代码看起来高级"——重载 new 是**正确性雷区**（item51/52 的常规全要守）
- "统一所有分配"——库代码抢全局 new 会污染宿主程序（→ item48 同理：全局钩子是应用的）

## HFT 关联

- 交易系统的正当理由排序实战：**②统计**（找偷分配的）常驻调试通道；
  **③性能**走"池化 + 启动预分配"（运行期零分配，→ 19.1 HFT）；
  **④对齐**在 C++17 用 `alignas` + `align_val_t`，自己重载的需求已消失
- 全局重载的统计模式只在**诊断构建**开——生产构建恢复默认
  （每分配一次的日志/栈回溯本身就成了延迟源，观测者效应，→ 06.6.5 ch04）
- NUMA 绑定的行情缓冲（网卡同节点内存）走 ⑤——
  用池化 + `mbind` 实现，别指望重载全局 new 能优雅做到

## 代码自测

**题目 1：** 五个替换理由中，哪两个"几乎零风险、应该常备"？为什么？

<details>
<summary>参考答案</summary>

**①检测错误 和 ②统计埋点**。
它们的共同点：**不改变分配语义**——guard bytes/日志只是"附加观察"，
内存照样来自 malloc，行为与默认分配器一致，错了最多是观察数据失真，
不会破坏程序逻辑。
而③④⑤都**改变分配语义**（内存从池/对齐分配器/共享内存来）——
对齐、尺寸、释放配对的正确性全部自己扛（item51/52 的常规），
写错就是堆损坏。先观察后改造：诊断通道常备，语义改造要 profile + 全套常规。

</details>

**题目 2：** 想优化分配性能，为什么说"先 LD_PRELOAD 换 jemalloc，再考虑手写"？

<details>
<summary>参考答案</summary>

① **风险**：jemalloc/tcmalloc 是十几年工业验证的分配器，
手写的池/重载要自己对齐 item51/52 的全部常规（循环、nullptr、尺寸、
placement 配对）——出错即堆损坏，且 bug 可能潜伏到特定尺寸/时序才爆；
② **覆盖**：LD_PRELOAD 一次替换**全程序**（含第三方库）的分配路径，
手写重载管不到没重编译的库；
③ **成本**：换预加载库是运维动作，不改一行代码——还能 A/B 对比。
手写重载的合理位置在**通用分配器之后**：profile 证明 jemalloc 仍不满足
（如固定尺寸单线程池可再快 10 倍）时，对**特定类型**做类内池化（item49）——
影响面最小、收益最集中。

</details>

**题目 3：** C++17 的 `new (std::align_val_t(64)) T` 解决了什么历史问题？

<details>
<summary>参考答案</summary>

C++17 前，`alignas(64)` 的类型用默认 `operator new` 只保证
`max_align_t`（通常 16B）对齐——超对齐类型（缓存行 64B、AVX-512 的 64B）
拿到的内存可能不对齐，重载 operator new 自己调 `aligned_alloc` 是唯一解法
（delete 也要配套，尺寸/对齐信息还得自己传）。
C++17 起：重载决议认得 `align_val_t`——编译器自动选带对齐参数的
operator new/delete 版本，**对齐成为语言级一等公民**，
"为了对齐而重载 new"这条理由基本退役。
读老代码看到手写的对齐分配器，先问：这是前 C++17 的遗产，
还是真有定制需求（NUMA/pmem）？

</details>
