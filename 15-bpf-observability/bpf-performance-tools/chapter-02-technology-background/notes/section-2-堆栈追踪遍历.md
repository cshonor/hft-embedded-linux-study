# 2. 堆栈追踪遍历 (Stack Trace Walking)

理解事件 **从哪条代码路径来** — `profile`、`offcputime`、栈采样都依赖栈回溯。

| 方法 | 原理 | 备注 |
|------|------|------|
| **帧指针 (Frame Pointer)** | x86-64：`RBP` 链 + 固定偏移 walk 栈帧 | **最快**；需 `-fno-omit-frame-pointer`（或 distro 默认保留） |
| **DWARF / debuginfo** | 调试信息解析栈 | 准但慢、需安装 debug 包 |
| **LBR** (Last Branch Record) | CPU 硬件记录最近分支 | 深度有限；Intel 常用 |
| **ORC** (Oops Rewind Capability) | 内核 unwind 元数据 | 内核栈常用；与用户态 DWARF 互补 |

> **HFT：** 发布二进制若 **省略帧指针**，火焰图会出现 `<unknown>` 或错误栈 — 与 [SysPerf Ch 6 CPU](../../../../14-systems-performance/chapter-06-cpus/) 的 `-g` / FPO 讨论同构。策略 SO 建议 **保留 frame pointer** 或配 USDT/静态探针。

```bash
# 检查内核是否启用 ORC（现代发行版常见）
grep CONFIG_UNWINDER_ORC /boot/config-$(uname -r)
```


### 常见陷阱

1. **栈 ID 过期导致栈信息错误** — BPF 用 stackid(Map) 返回栈的 ID 而非完整栈，Map 有大小限制，旧栈 ID 可能被新栈覆盖；用 `BPF_F_USER_STACK` 区分内核/用户栈
2. **忽视符号解析的依赖** — 栈 ID 只是地址，需要符号表（/proc/kallsyms、二进制 debug info）解析为函数名；strip 过的二进制无法解析用户栈
3. **在 HFT 热路径上频繁采栈** — 每次 stackid 调用有一定开销，高频 probe 上采栈会放大延迟；HFT 应用低频采样（如 99Hz）或按事件触发而非 per-hit

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BPF 如何获取和存储调用栈？**

   <details>
   <summary>参考答案</summary>

   BPF 程序调用 `bpf_get_stackid()` 将当前栈哈希后存入专用 Map，返回一个整型 ID。分析时用户态用该 ID 查 Map 获取地址列表，再通过符号表解析为函数名。这种方式避免每次都传完整栈数据到用户态。

   </details>

2. **栈 ID 过期是什么问题？如何缓解？**

   <details>
   <summary>参考答案</summary>

   stackid Map 有大小上限（默认有限），当栈数量超过上限时旧 ID 被覆盖，导致后续查询返回错误或错误栈。缓解：(1) 增大 Map；(2) 使用 `BPF_F_USER_STACK` / `BPF_F_KERNEL_STACK` 分别存储；(3) 接受少量丢失，关注高频栈。

   </details>

3. **为什么 strip 过的二进制无法解析用户态栈？**

   <details>
   <summary>参考答案</summary>

   符号解析需要地址到函数名的映射表，strip 操作删除了 .symtab 和 .strtab 段。解决方案：(1) 保留未 strip 版本用于分析；(2) 使用 USDT 探针替代栈追踪；(3) 用 DWARF debug info（-g 编译）。

   </details>

</details>

---
