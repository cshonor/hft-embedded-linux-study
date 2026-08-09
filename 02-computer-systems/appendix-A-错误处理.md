# 附录 A 错误处理 · Error Handling

> **CSAPP 3rd** · Bryant & O'Neill · **选读**

<!-- 章节笔记待补充：errno、错误检查惯例 -->

## 相关章节

- 上一章：[chapter-12-concurrent-programming/](./chapter-12-concurrent-programming/)

### 自测题

<details>
<summary>1. CSAPP 的错误处理包装函数(unix_error/system_error)做了什么？HFT 中应该怎么改进？</summary>

CSAPP 包装函数在系统调用返回 -1 时调用 `unix_error` 打印 `strerror(errno)` 并 `exit(1)`。确保错误不被忽略。

**HFT 改进**：1. 不能直接 `exit(1)`——交易系统需要优雅退出（撤单、保存状态）
2. 错误日志要带时间戳、线程 ID、上下文
3. 可恢复错误（EAGAIN/EINTR）要重试而非退出
4. 用 RAII 或 defer 模式确保资源释放
5. 关键错误（行情断连）要触发告警而非静默退出

</details>

