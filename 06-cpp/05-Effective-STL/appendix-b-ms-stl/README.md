# 附录 B：微软 STL 平台备注

**Remarks on Microsoft's STL Platforms**

## 本附录讲什么

MSVC 的 STL 实现（MS-STL）在迭代器调试、异常模式、安全检查上有自己的开关与行为差异。跨平台 C++ 工程需要了解这些差异，避免"在 GCC 能编译、在 MSVC 报错/行为不同"。

## 要点

- **`_HAS_ITERATOR_DEBUGGING`**：MSVC 的迭代器调试模式，开启后能检测越界 / 失效迭代器访问（开发期有用，性能开销大，发布关闭）。
- **`_SCL_SECURE_NO_WARNINGS`**：关闭不安全函数（`strcpy`/`sprintf`）的安全警告。HFT 代码应改用安全版本而非压制警告。
- **异常模式 `_HAS_EXCEPTIONS`**：MSVC 默认 `/EHsc`，异常开启；关闭异常时部分 STL 行为变化（如 `new` 抛 `bad_alloc` 变返回 `nullptr`）。
- **ABI 与 libstdc++/libc++ 不兼容**：MSVC 编译的 STL 容器不能与 GCC 编译的跨 DLL 边界传递——接口要 C ABI（裸指针/POD）。

## HFT 关联

HFT 引擎通常 Linux + GCC/Clang，MSVC 仅在 Windows 回测工具链。跨平台代码用 C ABI 边界隔离 STL 容器，避免 ABI 不兼容。

## 自测题

1. `_HAS_ITERATOR_DEBUGGING` 在什么阶段开启？发布为什么要关？
2. 为什么不能跨 MSVC/GCC 的 DLL 边界传递 `std::vector`？怎么解决？
3. 关闭异常后 `new` 失败的行为有何变化？
