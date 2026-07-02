# 3. 应用程序上下文与 USDT

**最可靠** 获取业务语义（SQL、ORM 操作、自定义事件）→ **USDT**。

### MySQL 示例

编译 **`-DENABLE_DTRACE=1`** 时提供 USDT，例如：

| 探针（示意） | 暴露 |
|--------------|------|
| `mysql:query__start` | 当前 **SQL** |
| `mysql:command__start` | 命令类型 |

```bash
# 列出 USDT（若已启用）
bpftrace -l 'usdt:/usr/sbin/mysqld:*'
```

**HFT：** 共置 **MySQL/ MariaDB** 审计慢查询；自研引擎可 **自建 USDT**（Ch 12 编译型思路）。

**原则：** USDT > 高频 uprobe；见 [Ch 2 § USDT](../../chapter-02-technology-background/)、[Ch 12](../../chapter-12-languages/)。

---
