# 7. MySQL 专用工具（案例）

### `mysqld_qslower`

内核侧过滤 **慢于阈值** 的查询 — 比慢查询日志 **低开销**，直接打印 **SQL 字符串**。

```bash
sudo mysqld_qslower-bpfcc 10   # 10ms 阈值示例
```

### `mysqld_clat`

按 **命令类型**（`COM_QUERY`、`COM_STMT_EXECUTE`…）的 **延迟直方图**。

```bash
sudo mysqld_clat-bpfcc 10
```

**迁移思路：** 任何带 USDT 的服务（Postgres、自研引擎）→ **慢路径 BPF 过滤 + 业务字段打印**。

---
