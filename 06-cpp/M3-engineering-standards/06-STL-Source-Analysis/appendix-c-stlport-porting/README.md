# 附录 C：STLPort 移植

**STLPort Porting**

## 要点

STLPort 是基于 SGI STL 的跨平台开源 STL 实现，曾在 MSVC/Borland 等非标准库环境下流行。本附录讲移植要点：

- **配置宏**：`_STLP_*` 系列宏控制线程模型、异常、调试模式。
- **线程安全**：`_STLP_THREADS` 开启多线程支持（配置器加锁）。
- **调试模式**：`_STLP_DEBUG` 检测迭代器失效/越界（类似 MSVC 的 `_HAS_ITERATOR_DEBUGGING`）。

## 现代意义

C++11 后主流编译器都自带标准库（libstdc++/libc++/MS-STL），STLPort 已基本退出历史舞台。本附录的价值在于理解"STL 实现的可移植性考量"——配置器线程模型、调试模式等概念在现代标准库中仍以不同形式存在。

## 自测题

1. STLPort 解决了什么历史问题？现代还需要它吗？
2. 配置器的线程安全由什么宏控制？与现代标准库的什么机制对应？
