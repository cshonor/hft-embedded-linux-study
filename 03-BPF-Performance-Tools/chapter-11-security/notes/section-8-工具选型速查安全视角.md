# 8. 工具选型速查（安全视角）

| 怀疑 | 工具 |
|------|------|
| 未知进程/脚本 | `execsnoop`、`elfsnoop` |
| Rootkit / 恶意 ko | `modsnoop` |
| 交互式攻击 | `bashreadline`、`ttysnoop` |
| 权限过大 | **`capable`** → 白名单 |
| 提权 | `setuids` |
| 越权探测 | `eperm` |
| 意外外连 | **`tcpconnect`** |
| 端口扫描 | `tcpreset` |
| 敏感文件访问 | **`opensnoop`** |

---
