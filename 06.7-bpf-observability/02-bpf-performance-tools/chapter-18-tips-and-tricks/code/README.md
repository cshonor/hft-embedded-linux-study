# 02-ch18 code · 排错技巧

> 《BPF Performance Tools》第 18 章（全书收尾）示例落盘。
> 笔记中 getaddrinfo/man 引用（§2 块2/3）保留在笔记内。

## scripts/

| 脚本 | 出处 | 内容 | 验证 |
|------|------|------|------|
| [debug-tips.sh](./scripts/debug-tips.sh) | §2/3/5 | 排错三招+一坑：①事件对账法（dd 已知负载 vs funccount）②事件缺失排查（perf stat 证伪 + perf probe 造点）③反馈回路陷阱（输出别写被监控对象）④ring buffer 丢事件 | `bash -n` ✓ |

## 核心认知

**你的观测本身也可能说谎。** 对账法（①）是唯一让人安心的手段：
制造已知负载，看工具报数是否一致——每次上新探针都该先过这一关。
