# 26-parallel — 并行任务执行器

**触发**：学完 `chapter-26-monitoring-child-processes` 笔记后开写。
**前置章节**：Ch24（fork）、Ch26（waitpid、SIGCHLD、WNOHANG 轮询 vs 阻塞）。

## 目标

`parallel [-j N] "cmd1" "cmd2" ...` 最多 N 个子进程并发执行所有命令。

## 需求清单

- [ ] `-j` 限制并发度（默认 CPU 核数）
- [ ] SIGCHLD 驱动：任一子进程退出立刻补位下一个任务
- [ ] 逐条记录每个任务的 pid、退出码/终止信号、耗时
- [ ] 全部回收后汇总打印成功/失败计数

## 验收标准

- `parallel -j 2 "sleep 1" "sleep 1" "sleep 1"` 总耗时约 2 秒而非 3 秒
- 无论任务以何种方式结束，无僵尸、无漏回收

## HFT / 嵌入式关联

这就是进程池模型；策略研究批量回测、数据预处理流水线都可以直接复用这个骨架。
