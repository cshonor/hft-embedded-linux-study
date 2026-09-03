# Ch3 内存类：越界 / 泄漏 / use-after-free

> 🔴 精读 · 内存「越用越多」或「偶发崩」怎么办

**这一章解决什么症状**：内存持续增长、偶发崩溃、值莫名被改、未初始化读——「内存管理错误」这一类问题。内存 bug 的阴险在于：越界写、use-after-free 往往**不立即崩溃**，而是潜伏到很久之后在毫不相关的地方爆出来。

本章工具：valgrind（memcheck）动态分析、ASan/UBSan 编译期插桩——一个准但慢、一个快但需重编译。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 3.1 valgrind memcheck（泄漏 / 越界 / UAF 精确定位） | （待写） |
| 3.2 AddressSanitizer（ASan 快速内存错误检测） | （待写） |
| 3.3 UndefinedBehaviorSanitizer（UBSan 未定义行为） | （待写） |

---

## HFT 关联

- **7×24 长跑进程最怕慢泄漏**：交易进程内存缓慢增长，几周后 OOM 崩溃，`valgrind memcheck` 定位到具体分配点；
- **越界是崩溃的潜伏根因**：一块内存被写坏，可能几十万次调用后才在别处崩溃，ASan 在开发期就把它钉死；
- **valgrind 定性 → ASan 复测**：valgrind 慢但无需重编，先定性；确认后 ASan 重编译快速回归。
