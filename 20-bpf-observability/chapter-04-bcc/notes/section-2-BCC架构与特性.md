# 2. BCC 架构与特性

### 编译与加载流程

```
用户脚本 (Python 等)
    → 嵌入 BPF C 源码
    → Clang/LLVM 编译为 BPF 字节码
    → bpf() 系统调用加载程序 + 创建 Map
    → 附加到 kprobe/uprobe/tracepoint/USDT 等
    → 用户态轮询/读取 Map，格式化输出
```

### 内核级能力

| 能力 | 典型用途 |
|------|----------|
| **动态 kprobes / uprobes** | 任意内核/用户函数插桩（需符号） |
| **Tracepoint** | 稳定、低开销的内核静态探针 |
| **BPF Map** | 直方图、频率计数、聚合 — **海量事件在内核汇总** |
| **栈回溯** | `bpf_get_stackid` + 栈 Map → `stackcount` / `profile` |

### 用户级能力

| 能力 | 说明 |
|------|------|
| **USDT** | 用户态静态探针（需应用带 SDT 探针，如某些数据库/语言运行时） |
| **debuginfo 符号解析** | 内核/用户栈、函数名 — 依赖 debug 包或 BTF |
| **Python 胶水** | 参数解析、输出格式化、与 CLI 集成 |

```bash
# 常见安装名（发行版差异）：bcc-tools / python3-bcc
ls /usr/share/bcc/tools/ | head
man opensnoop-bpfcc    # 或 bcc-opensnoop 等，视发行版而定
```

→ 自研工具深入：[appendix-C-BCC工具开发.md](../../appendix-C-BCC工具开发.md) · C/libbpf 路线：[appendix-D-C语言BPF.md](../../appendix-D-C语言BPF.md)


### 常见陷阱

1. **忽视 BCC 运行时编译的启动延迟** — BCC 每次运行都用 Clang 编译 BPF C 代码，启动有 1-3 秒延迟；对 HFT 的快速排障（秒级响应）有影响，bpftrace 启动更快
2. **混淆 BCC 和 libbpf+CO-RE 的部署模型** — BCC 需要目标机器有 Clang+内核头文件；libbpf+CO-RE 预编译为单一二进制，无需编译器——部署更简单
3. **以为 BCC 工具修改 BPF 程序不需要重启** — BPF 程序一旦加载就运行在内核中，修改需重新编译加载（重启工具）；不能热修改已加载的 BPF 逻辑

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BCC 的运行时编译模型有什么优缺点？**

   <details>
   <summary>参考答案</summary>

   优点：(1) 可用内核头文件中的任意类型和宏；(2) 修改 BPF C 代码后直接运行，无需预编译。缺点：(1) 需要目标机器安装 Clang + 内核头文件（部署重）；(2) 启动有 1-3 秒编译延迟；(3) 编译错误在运行时才暴露。

   </details>

2. **BCC 和 libbpf+CO-RE 的部署模型有什么区别？**

   <details>
   <summary>参考答案</summary>

   BCC：目标机器需要 Clang + LLVM + kernel-headers，运行时编译，部署重但灵活。libbpf+CO-RE：预编译为单一二进制（含 BTF 重定位信息），目标机器无需编译器，部署轻但需要内核支持 BTF。新工具链渐迁 libbpf+CO-RE。

   </details>

3. **HFT 场景中 BCC 的启动延迟如何影响排障？**

   <details>
   <summary>参考答案</summary>

   BCC 工具启动有 1-3 秒编译延迟，如果延迟尖刺只持续毫秒级，等 BCC 编译完现象可能已消失。对策：(1) 预先启动工具用 interval 模式持续监控；(2) 短暂现象用 bpftrace（启动更快）；(3) 复杂工具可预编译为 BPF 字节码避免运行时编译。

   </details>

</details>

---
