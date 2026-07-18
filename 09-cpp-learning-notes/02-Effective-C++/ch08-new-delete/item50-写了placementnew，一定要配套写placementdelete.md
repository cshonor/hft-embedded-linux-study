# 条款 50：写了 placement new，一定要配套写 placement delete

## 本节讲什么

构造抛异常时，编译器会调用对应版本 placement delete 回收内存，只写 new 不写 delete 会内存泄漏。

## 示例

```cpp
void *operator new(std::size_t, void *place) { return place; }
void operator delete(void *, void *) noexcept {}  // placement delete
```
