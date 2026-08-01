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


---

← [本章导读](../README.md)
