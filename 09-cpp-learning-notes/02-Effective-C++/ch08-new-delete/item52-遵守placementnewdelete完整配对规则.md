# 条款 52：遵守 placement new/delete 完整配对规则

## 本节讲什么

不同参数签名的分配释放函数必须一一对应。

## 示例

```cpp
alignas(std::max_align_t) char buf[sizeof(T)];
T *p = new (buf) T(args);
// ...
p->~T();  // placement new 不自动析构
```
