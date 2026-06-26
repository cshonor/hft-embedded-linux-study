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

---
