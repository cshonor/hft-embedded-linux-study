# 28-msh — mini-shell（三阶段，跨 Ch28/34/44）

**触发**：v1 在学完 `chapter-28-process-creation-exec-detail` 后开写；v2、v3 分别等 Ch34、Ch44。

## 三阶段规划

| 阶段 | 触发章 | 新增能力 | 核心知识点 |
|---|---|---|---|
| v1 | Ch28 | 执行外部命令 + `>` `<` `>>` 重定向 | fork、execvp、dup2、argv 解析 |
| v2 | Ch34 | 前后台与作业控制 | setpgid、tcsetpgrp、SIGTSTP/SIGCHLD、SIG_IGN 抢救 |
| v3 | Ch44 | 多级管道 | pipe、dup2 串联、逐段 fork |

## 各阶段验收标准

- [ ] **v1**：`msh> ls -l > out.txt` 正确；`cd/exit` 内建可用
- [ ] **v2**：`Ctrl+C` 只杀前台作业；`sleep 100 &` 后 `jobs`/`fg`/`bg` 可用；`Ctrl+Z` 挂起前台
- [ ] **v3**：`cat file | grep x | wc -l` 结果与系统 shell 一致

## 已知陷阱（写的时候重点验证）

- 交互 shell 必须忽略 SIGINT/SIGQUIT/SIGTSTP，子进程再恢复默认——顺序错了 shell 自己会被杀
- v2 中后台作业读终端会收到 SIGTTIN，观察并处理
- 管道右端先 fork 还是先 pipe，影响哪端会拿到多余的写端 fd

## 说明

代码目录按阶段建 `phase1/ phase2/ phase3/`，各阶段独立可编译，不回改上一阶段。
