# 19-dirwatch — 目录实时监控器

**触发**：学完 `chapter-19-monitoring-file-events` 笔记后开写。
**前置章节**：Ch19（inotify）、Ch63 预习（事件循环的雏形）。

## 目标

`dirwatch <dir>` 实时打印目录下文件的创建/删除/修改/改名事件。

## 需求清单

- [ ] `inotify_init1()` + `inotify_add_watch()`，监视 `IN_CREATE|IN_DELETE|IN_MODIFY|IN_MOVED_FROM|IN_MOVED_TO`
- [ ] 递归监控：新建子目录自动加 watch
- [ ] 事件名、文件路径、时间戳（`clock_gettime(CLOCK_REALTIME)`）格式化输出
- [ ] Ctrl+C 干净退出（关闭 fd，无残留）

## 验收标准

- 终端 `touch/rm/mv/echo >>` 时，毫秒级打印对应事件
- 新建目录后其内文件事件也能被捕获

## HFT / 嵌入式关联

配置热加载、固件升级包落盘检测都是这个模式；事件驱动的骨架（fd 等待 + 分发）就是 epoll 循环的前身。
