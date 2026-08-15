# 14.3 两种打包形态 demo

笔记：[14.3.1-Workspace与微服务.md](../14.3.1-Workspace与微服务.md)

| Package | 形态 | 产物 |
|---------|------|------|
| `common/` | lib only | **无 exe** |
| `app_bin/` | bin → 链 common | **1 个 exe**（形态 1） |
| `feed/` · `trade/` | 各 bin → 链 common | **各 1 exe**（形态 2，多进程） |

```bash
cargo run -p app_bin
cargo run -p feed
cargo run -p trade
```
