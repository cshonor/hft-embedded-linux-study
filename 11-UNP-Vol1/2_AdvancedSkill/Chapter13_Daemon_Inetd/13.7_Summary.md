# 13.7 小结

> [study.md](../study.md)

---

## 章节核心提炼

### 1. 守护进程

- **无控制终端**的后台基石  
- 手写规范流程：**fork → setsid → fork → chdir/umask → 关闭并重定向 I/O**（`daemon_init`）

### 2. 日志隔离

- 失去终端 = 失去屏幕  
- **`syslog`** 是守护进程合法、安全的输出

### 3. inetd 超级服务器

- **select** 统一监听多服务  
- **fork + dup2(0,1,2) + exec** — 解释为何部分老服务源码里**没有 `accept`**

---

## 与现代部署

| 经典 UNP | 常见现状 |
|----------|----------|
| inetd | systemd socket activation、独立守护进程 |
| syslogd | rsyslog/journald |
| daemon_init 步骤 | 仍适用于自写 C 守护进程；也可用 `daemon(3)`（非 POSIX） |

---

## 个人学习总结

（待填）
