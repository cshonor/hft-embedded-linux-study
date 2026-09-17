# 23-mytimeout — 命令超时器

**触发**：学完 `chapter-23-timers-sleeping` 笔记后开写。
**前置章节**：Ch20–22（信号）、Ch24–26（fork/waitpid）。

## 目标

复刻 coreutils 的 `timeout`：`mytimeout <秒> <命令> [参数...]`，超时则杀掉命令。

## 需求清单

- [ ] fork + execvp 执行命令
- [ ] `timerfd_create(CLOCK_MONOTONIC)` 设超时（或 alarm，二选一并在 README 说明理由）
- [ ] 超时后先发 SIGTERM，宽限 2 秒后仍存活则 SIGKILL
- [ ] `waitpid` 回收，转发子进程退出码；被信号杀掉时报告 `killed by SIGxxx`

## 验收标准

- `mytimeout 2 sleep 10` → 2 秒后退出，无僵尸进程
- `mytimeout 2 ls` → 不到 2 秒即正常结束，退出码与 `ls` 一致

## HFT / 嵌入式关联

行情源进程看门狗、采集任务限时执行，本质都是 timerfd + SIGCHLD 的组合。
