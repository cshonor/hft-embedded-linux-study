# 附录 B：网站资源

**Web Resources**

## 要点

- **SGI STL**：`sgi.com/tech/stl` — 最经典的 STL 在线文档与源码。
- **cppreference.com** — 当前最权威的 C++ 标准库在线参考（含 C++17/20 更新，推荐日常查询）。
- **cplusplus.com** — 教程式参考（较浅，适合入门）。
- **gcc/libstdc++ 源码**：读 `<bits/stl_*.h>` 理解实现。
- **llvm/libc++ 源码**：对照另一种实现。

## HFT 实践

读标准库源码（libstdc++）是理解 STL 性能细节的最直接途径——cppreference 给契约，源码给实现。HFT 调优时常需翻 `stl_vector.h`/`hashtable` 源码确认内存模型。
