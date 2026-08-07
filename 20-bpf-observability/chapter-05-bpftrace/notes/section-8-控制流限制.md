# 8. 控制流限制

BPF **验证器** 要求程序 **有界、可终止** — 禁止内核死循环。

| 允许 | 禁止 |
|------|------|
| `/filter/` 布尔过滤 | `while (1)` 等无限循环 |
| `if / else` | 无界 `for` |
| 三元 `? :` | 递归 |
| `unroll(N) { ... }` | N 必须 **编译期常量** |

```bash
bpftrace -e '
kprobe:foo
{
    unroll(4) {
        @ = count();
    }
}
'
```

**实践：** 复杂状态机用 **多个探针 + map** 拆分，而非循环。


### 常见陷阱

1. **写无限循环导致 verifier 拒绝** — BPF verifier 要求循环有确定的上界（bounded loop），无限循环会被拒绝；bpftrace 的 while/until 需要有明确退出条件
2. **以为 bpftrace 支持函数定义** — bpftrace 不支持用户自定义函数（只支持内置函数和内联表达式）；复杂逻辑需拆分为多个 probe 或用 Map 关联
3. **在 probe 块中做复杂条件判断** — 复杂 if-else 嵌套会增加 verifier 验证路径数，可能超限；应尽量用 filter 在进入 probe 前过滤

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 的控制流有哪些限制？为什么？**

   <details>
   <summary>参考答案</summary>

   (1) 循环必须有确定上界（verifier 要求 bounded loop）；(2) 不支持用户自定义函数；(3) 不支持 goto；(4) if/else 嵌套深度有限（verifier 路径爆炸）。这些限制源于 BPF verifier 的安全保证——必须在有限时间内确认程序会终止且不越界。

   </details>

2. **为什么 BPF verifier 要求循环有上界？**

   <details>
   <summary>参考答案</summary>

   Verifier 通过模拟执行来验证安全性（检查所有可能的路径），无限循环会让模拟永不终止。Linux 5.3+ 支持 bounded loop（verifier 能推断循环次数上限），但仍不支持无限循环。替代方案：用 unroll 编译指令或 Map + interval 模式实现「循环效果」。

   </details>

3. **复杂条件判断应该放在 filter 还是 actions 中？为什么？**

   <details>
   <summary>参考答案</summary>

   尽量放在 filter（`/condition/`）中。Filter 在进入 probe 块前求值，不命中的事件直接跳过，不执行 BPF 程序体。放在 actions 中的 if-else 每次命中都执行完整 BPF 程序再分支，增加开销和 verifier 验证路径。原则：过滤条件放 filter，业务逻辑放 actions。

   </details>

</details>

---
