## 5.6 常见陷阱（Gotchas）

### Missing Symbols（缺失符号）

火焰图 / perf report 出现 `[unknown]` 或 `0x7f...` 地址：

| 原因 | 解决 |
|------|------|
| **strip** 了符号表 | 编译加 `-g`，发布用 **split debuginfo** |
| 动态库无 debuginfo | 安装 `-dbg` / `-debuginfo` 包 |
| **JIT**（Java、Node） | `perf-map-agent`、`-XX:+PreserveFramePointer` |

### Missing Stacks（缺失堆栈）

栈断层 → 火焰图「平头」、深度不够：

| 原因 | 解决 |
|------|------|
| **省略帧指针**（`-fomit-frame-pointer`） | 编译 `-fno-omit-frame-pointer`（或 `-mno-omit-leaf-frame-pointer`） |
| 栈太深 / 采样限制 | 增大 `--call-graph fp` 深度 |
| **inline 过多** | 权衡 `-O3` 与可观测性 |

**HFT 发布构建建议：**

```
Release：-O3 -g -fno-omit-frame-pointer
Debug symbols：单独 debug 包，生产按需挂载
危机 perf：永远能采到可读的 strategy 栈
```

---


### 常见陷阱

1. 编译去掉帧指针——-O2 默认 -fomit-frame-pointer，perf 栈回溯全是 [unknown]
2. strip 掉符号表——生产二进制 strip 后 perf report 看不到函数名，应保留 debuginfo
3. inline 过度——-O3 激进 inline 导致函数太大影响 I-cache，且火焰图栈变浅看不清层次

<details>
<summary>自测题（点击展开）</summary>

1. 为什么 HFT Release 构建要保留帧指针？
   <details><summary>答</summary>perf 栈回溯需要帧指针（-fno-omit-frame-pointer）——去掉后栈全是 [unknown] 无法分析</details>
2. strip 符号表有什么后果？
   <details><summary>答</summary>perf report 看不到函数名——应保留 debuginfo 包或不在生产二进制上 strip</details>
3. -O3 aggressive inline 有什么副作用？
   <details><summary>答</summary>函数过大影响 I-cache（icache miss），且火焰图栈变浅看不清调用层次</details>

</details>


---

← [本章导读](../README.md)
