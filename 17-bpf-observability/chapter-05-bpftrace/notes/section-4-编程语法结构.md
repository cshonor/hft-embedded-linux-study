# 4. 编程语法结构

基本形式：

```c
probes /filter/ { actions }
```

| 部分 | 说明 | 示例 |
|------|------|------|
| **Probes** | 事件触发点 | `kprobe:do_sys_open` |
| **Filter** | 可选布尔条件，为真才执行动作 | `/pid == 12345/` |
| **Actions** | `{ }` 内语句，分号分隔 | `@ = count();` |

**完整示例：**

```bash
bpftrace -e '
kprobe:vfs_read
/pid == $1/
{
    @bytes = sum(arg2);
}
' $(pidof myapp)
```

| 语法糖 | 含义 |
|--------|------|
| `BEGIN` | 脚本启动时执行一次（初始化） |
| `END` | 脚本退出前执行（收尾打印） |
| `interval:s:5` | 每 5 秒触发（看实时计数） |


### 常见陷阱

1. **filter 写错导致无输出或全输出** — filter 是布尔表达式，写 `pid = 1234`（赋值）而非 `pid == 1234`（比较）是常见错误；bpftrace 对此不一定报错
2. **在 actions 中写复杂控制流** — bpftrace 支持 if/else 和有限循环，但复杂逻辑应拆分为多个 probe 或用 Map 聚合；verifier 会拒绝过深的逻辑
3. **忽视多条语句的分号** — actions 中每条语句必须以分号结尾；漏分号在某些版本会报语法错误

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 的基本语法形式是什么？**

   <details>
   <summary>参考答案</summary>

   `probe /filter/ { actions }`。probe 指定事件源（如 `kprobe:vfs_read`），filter 是可选的布尔条件（如 `/pid == 1234/`），actions 是花括号内以分号分隔的语句（如 `{ @bytes = sum(arg2); }`）。

   </details>

2. **filter 表达式中 `=` 和 `==` 有什么区别？写错了会怎样？**

   <details>
   <summary>参考答案</summary>

   `==` 是比较运算符，`= `是赋值。`/pid == 1234/` 正确——只追踪 PID 1234。`/pid = 1234/` 错误——会尝试赋值，可能不报错但行为不可预期（filter 恒真或编译警告）。始终用 `==` 做比较。

   </details>

3. **bpftrace 的语法糖 BEGIN/END/interval 各适合什么场景？**

   <details>
   <summary>参考答案</summary>

   BEGIN：初始化（`@start = nsecs;` 记录基准时间、打印表头）。END：收尾（`print(@latency)` 自定义输出格式、计算 `(@end - @start) / 1000000` 总耗时）。interval:s:5：定期输出（每 5 秒 `print(@count); clear(@count)` 实现滚动窗口统计）。

   </details>

</details>

---
