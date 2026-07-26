# 第 13 章：守护进程和 inetd 超级服务器（厚版）

> [Ch 11](../Chapter11_Name_Address_Convert/study.md) · **Ch 13** · [Ch 14](../Chapter14_AdvancedIO_Func/)（待笔记）  
> 逐节：`13.x_*.md`

> **说明**：上传 PDF 可视内容截至第 8 章；第 13 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握 **守护进程**化步骤、`syslog` 日志、**inetd** 的 select/fork/dup2/exec 模型，以及 **inetd 子服务**与自守护进程的差异。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 13.1 | [13.1_Overview](./13.1_Overview.md) | 守护进程定义与启动方式 |
| 13.2 | [13.2_syslogd_Daemon](./13.2_syslogd_Daemon.md) | 中心化日志守护进程 |
| 13.3 | [13.3_Syslog_Func](./13.3_Syslog_Func.md) | syslog/openlog |
| 13.4 | [13.4_Daemon_Init_Func](./13.4_Daemon_Init_Func.md) | **daemon_init 六步** |
| 13.5 | [13.5_inetd_Daemon](./13.5_inetd_Daemon.md) | 超级服务器工作流 |
| 13.6 | [13.6_Daemon_Inetd_Func](./13.6_Daemon_Inetd_Func.md) | inetd 子服务规范 |
| 13.7 | [13.7_Summary](./13.7_Summary.md) | 全章收束 |

---

## 一章速记

```text
守护进程：无控制终端；fork→setsid→fork→chdir("/")→umask(0)→0/1/2→/dev/null
日志：syslog + openlog；勿 printf（尤其 inetd 子进程：1/2=套接字）
inetd：inetd.conf → 多端口 select → accept → fork → dup2(0,1,2) → exec
inetd 服务：不 socket/accept；getpeername(0)；只 syslog
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 4 | fork、dup2、exec |
| Ch 5 | SIGHUP、信号处理 |
| Ch 6 | inetd 的 select 多路复用 |
| Ch 8 | syslogd UDP 514 |
| Ch 11 | 自守护进程仍可用 getaddrinfo 监听 |

---

## 两种服务写法对照

| | 自守护（13.4） | inetd 托管（13.5–13.6） |
|--|----------------|-------------------------|
| 终端 | 自行脱离 | inetd 已处理 |
| 监听 | socket/bind/listen/accept | **无**；0/1/2=连接 |
| 错误输出 | syslog（推荐） | **仅** syslog，禁止 stderr |
