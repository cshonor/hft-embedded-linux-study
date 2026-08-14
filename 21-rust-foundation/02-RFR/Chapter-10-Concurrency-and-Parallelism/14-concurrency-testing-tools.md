# 5.3 Use Concurrency Testing Tools（并发测试工具）

> 所属：**Sane Concurrency** · [← 章索引](./README.md)

| 工具 | 作用 |
|------|------|
| **Loom** | 系统化探索线程交错；接管 `Mutex`/原子语义 |
| **ThreadSanitizer (TSan)** | 运行时数据竞争检测；CI 可用（看平台支持） |
| **Miri** | unsafe / 别名 UB → [第 9 章](../Chapter-09-Unsafe-Code/12-check-your-work.md) |

→ [第 6 章 · 测试增强](../Chapter-06-Testing/06-test-augmentation.md)
