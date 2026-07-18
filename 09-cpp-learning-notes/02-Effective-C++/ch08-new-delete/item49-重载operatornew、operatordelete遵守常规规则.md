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
