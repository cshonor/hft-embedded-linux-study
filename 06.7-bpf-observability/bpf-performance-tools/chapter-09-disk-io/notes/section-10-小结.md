# 10. 小结（9.6）

> 底本：《BPF之巅》第 9 章 磁盘 I/O，9.6 节（印刷 p410）

本章展示了用 BPF 跟踪**整个存储 I/O 软件栈各层**：

| 层 | 工具 |
|----|------|
| 块 I/O 层 | biolatency、biosnoop、biotop、bitesize、seeksize、biopattern、biostacks、bioerr |
| I/O 调度器 | iosched |
| md 卷管理 | mdflush |
| SCSI 驱动 | scsilatency、scsiresult |
| NVMe 驱动 | nvmelatency |

## 本章方法论沉淀

1. **先分布后个例**：biolatency（分布/多峰）→ biosnoop（逐事件模式）
2. **拆时长**：请求时长 = 等待（排队）+ 服务；biosnoop QUE / iosched / biolatency -Q 各有侧重
3. **归因要跨层**：块层只有 kworker 时回第 8 章（页缓存层）找进程；biostacks 直接给内核栈翻译因果
4. **错误先量化再定性**：bioerr 的 USB 探测案例——频率、来源、错误码三者齐了才下结论
5. **没有跟踪点也能写工具**：nvmelatency 的 funccount 摸底 → 源码定边界 → 参考新版跟踪点实现读结构体

## 下钻链（承上启下）

```
Ch8 文件系统层（fileslower / cachestat / writeback）
   ↓ 缓存 miss / 同步写落盘
Ch9 块层（biolatency → biosnoop → biostacks → iosched → 驱动层）
   ↓
Ch10 网络（另一类 I/O）
```

第 8→9 章打通后，"应用读慢"的完整归因路径已经齐备：是页缓存 miss？是排队？是设备慢？是错误重试？逐层可证。
