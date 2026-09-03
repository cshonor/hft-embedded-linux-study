# Ch3 内存类：越界 / 泄漏 / use-after-free

> 🔴 精读 · 内存「越用越多」或「偶发崩」怎么办

**这一章解决什么症状**：内存持续增长、偶发崩溃、值莫名被改、未初始化读——「内存管理错误」这一类问题。内存 bug 的阴险在于：越界写、use-after-free 往往**不立即崩溃**，而是潜伏到很久之后在毫不相关的地方爆出来。

本章工具：valgrind（memcheck）动态分析、ASan/UBSan 编译期插桩——一个准但慢、一个快但需重编译。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 3.1 valgrind memcheck（泄漏 / 越界 / UAF 精确定位） | [01-valgrind-memcheck.md](notes/01-valgrind-memcheck.md) |
| 3.2 AddressSanitizer（ASan 快速内存错误检测） | [02-addresssanitizer.md](notes/02-addresssanitizer.md) |
| 3.3 UndefinedBehaviorSanitizer（UBSan 未定义行为） | [03-undefinedbehaviorsanitizer.md](notes/03-undefinedbehaviorsanitizer.md) |

## 工具选型速记

| 工具 | 原理 | 开销 | 需重编译 | 抓什么 |
|------|------|------|----------|--------|
| valgrind memcheck | 动态二进制翻译 + 影子内存 | 20–50× | ❌ | 越界 / UAF / 泄漏 / 未初始化值 |
| ASan | 编译期插桩 + 红区/隔离区 | ~2× | ✅ | 越界（含栈）/ UAF / 泄漏 |
| UBSan | 编译期插桩 | ~1.2× | ✅ | 有符号溢出 / 移位 / 除零等 UB |

> 分工口诀：ASan 管「地址」、valgrind 管「地址+值」（兜底）、UBSan 管「运算语义」。开发期默认 ASan+UBSan 全家桶，无源码二进制用 valgrind 定性。

---

## HFT 关联

- **7×24 长跑进程最怕慢泄漏**：交易进程内存缓慢增长，几周后 OOM 崩溃，`valgrind memcheck` 定位到具体分配点；
- **越界是崩溃的潜伏根因**：一块内存被写坏，可能几十万次调用后才在别处崩溃，ASan 在开发期就把它钉死；
- **valgrind 定性 → ASan 复测**：valgrind 慢但无需重编，先定性；确认后 ASan 重编译快速回归。
