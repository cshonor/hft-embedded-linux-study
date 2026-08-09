## ④ 引发 Bug 与打印信息

| 宏/函数 | 行为 |
|---------|------|
| **`BUG()` / `BUG_ON(cond)`** | 条件真 → **故意 Oops** · 栈回溯 · 终止当前操作 |
| **`panic()`** | **更致命** — 打印后 **挂起整机** |
| **`dump_stack()`** | 只 **打印栈** — **不杀进程、不 panic** — 日常路径跟踪 |

```c
BUG_ON(ptr == NULL);           /* 绝不应发生 */
if (debug) dump_stack();       /* 我在哪？ */
```

| 选用 | |
|------|--|
| 开发断言 | `BUG_ON` |
| 生产可恢复 | `WARN_ON`（书中相关）/ 错误码返回 |
| 跟踪流 | `dump_stack` |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** BUG_ON() 和 WARN_ON() 的区别？HFT 驱动该用哪个？

<details><summary>答案</summary>

BUG_ON(cond)：条件为真时触发 oops/panic（致命错误，不可继续）。WARN_ON(cond)：条件为真时打印调用栈但继续运行（警告，可能可恢复）。HFT 驱动规则：1) 硬件状态不一致 → BUG_ON（不可信任硬件状态）；2) 参数非法 → WARN_ON + 返回错误码（可恢复）。生产环境避免 BUG_ON 在热路径（会 panic 整个系统）。

</details>

</details>
---
