# 条款 49：重载 operator new、operator delete 遵守常规规则

## 本节讲什么

内存对齐、处理 0 字节分配、失败抛出 `bad_alloc` 或者返回空指针，兼容标准行为。

## 示例

```cpp
void *operator new(std::size_t sz) {
    if (void *p = std::malloc(sz)) return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
```

---

## 代码自测

**题目 1：** 什么情况下需要替换全局 `operator new`/`operator delete`？

<details>
<summary>参考答案</summary>

常见原因：1) 检测内存泄漏（在 new/delete 中记录分配/释放）；2) 提高性能（用内存池减少 malloc 开销）；3) 统计内存使用量；4) 增加对齐保证（如 16 字节对齐）。替换后必须遵守约定：正确处理 `new(0)`（应分配 1 字节）、循环重试 + new-handler、异常安全。

</details>
