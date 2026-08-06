# 条款 28：不要返回指向对象内部成员的句柄（指针/引用）

## 本节讲什么

外部拿到内部成员引用，可以绕过 `private` 修改内部状态，破坏封装；临时对象引用还会悬垂。

## 示例

```cpp
class String {
    char *data;
public:
    const char &operator[](int i) const { return data[i]; }
    // 不要返回 char& 的非 const 版本给 const 对象外的句柄
};
```
