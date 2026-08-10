# 7.1 Oops 是什么 / panic vs oops

> 🔴 精读 · Part 3: Diagnostics & Advanced Tools

## 本节要点

### Oops vs Panic

| 特性 | Oops | Panic |
|------|------|-------|
| 触发 | 非致命错误 (NULL deref, 越界等) | 致命错误 / panic() 主动调用 |
| 系统状态 | 继续运行 (杀死当前进程) | 系统停止 |
| 后续 | 可继续操作 (但不安全) | 必须重启 |
| 配置 | `CONFIG_BUG=y` | `panic_on_oops=1` 可升级为 panic |

### Oops 触发条件

- NULL 指针解引用
- 非法内存访问（未映射地址）
- 非法指令
- 断言失败 (BUG_ON / WARN_ON)
- 栈溢出

### 控制 Oops 行为

```bash
# Oops 后是否 panic
cat /proc/sys/kernel/panic_on_oops    # 0=继续, 1=panic
echo 1 > /proc/sys/kernel/panic_on_oops  # 生产环境常设 1

# Panic 后自动重启延迟
cat /proc/sys/kernel/panic    # 秒数, 0=不重启
echo 10 > /proc/sys/kernel/panic  # 10 秒后自动重启
```

### HFT 关联

HFT 生产环境应设 `panic_on_oops=1` + `panic=5`——Oops 后 5 秒自动重启，避免在不确定状态下继续交易。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** Oops 后系统继续运行为什么不安全？

> Oops 意味着内核数据结构可能已损坏（如链表断裂、引用计数错误）。继续运行可能导致级联崩溃、数据损坏或安全漏洞。生产环境建议设 `panic_on_oops=1` 让系统重启到已知良好状态。

</details>
