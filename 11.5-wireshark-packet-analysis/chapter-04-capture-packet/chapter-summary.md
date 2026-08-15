# 第4章 玩转捕获数据包

> 全书：[../README.md](../README.md) · 上一章：[第3章 Wireshark 入门](../chapter-03-wireshark-intro/chapter-summary.md)

## 整体框架

```text
4.1 捕获文件（pcapng · 导出子集 · 合并）
        ↓
4.2 查找 / 标记 / 打印
        ↓
4.3 时间格式 · *REF* · Time Shift
        ↓
4.4 Capture Options（Input / Output·Ring / 实时更新）
        ↓
4.5 捕获过滤器 BPF vs 显示过滤器
```

| 小节 | 主题 | 文件 |
|------|------|------|
| 4.1 | 保存、导出、合并 | [01-use-capture-files.md](./01-use-capture-files.md) |
| 4.2 | 查找、标记、打印 | [02-analyze-packets.md](./02-analyze-packets.md) |
| 4.3 | 时间与偏移 | [03-time-display-format.md](./03-time-display-format.md) |
| 4.4 | 高级捕获选项 | [04-advanced-capture-options.md](./04-advanced-capture-options.md) |
| 4.5 | 过滤器基础 | [05-filter-basics.md](./05-filter-basics.md) |

## 重点难点

| 点 | 说明 |
|----|------|
| **pcapng** | 默认格式；多接口与元数据优于 pcap |
| **Export Specified** | 胖文件瘦身；Marked / Displayed |
| **Time Reference** | `*REF*` 归零看后续时序 |
| **Time Shift** | 多探针时钟不同步时合并前必考虑 |
| **Ring Buffer** | FIFO 覆盖；防磁盘打满 |
| **实时更新** | 高流量时默认关 |
| **捕获 vs 显示滤** | BPF 真不抓 vs 仅隐藏 |
| **tcp&4==4** | 捕获 RST 的 BPF；显示用 `tcp.flags.reset` |

## 实操要点

1. 习惯 **pcapng** 命名归档；故障包 **Export Displayed/Marked** 再传阅。
2. 设一次 **baseline** 的 Time Reference 练习相对延迟。
3. 长跑抓包：**Ring Buffer** 或 File Sets + 捕获 `host`。
4. 显示滤熟练后把常用式加入工具栏；全文速查 [cheatsheet/notes.md](../cheatsheet/notes.md)。

## 小节索引

- [4.1 使用捕获文件](./01-use-capture-files.md)
- [4.2 分析数据包](./02-analyze-packets.md)
- [4.3 时间显示格式与相对参考](./03-time-display-format.md)
- [4.4 高级捕获选项](./04-advanced-capture-options.md)
- [4.5 过滤器基础](./05-filter-basics.md)
