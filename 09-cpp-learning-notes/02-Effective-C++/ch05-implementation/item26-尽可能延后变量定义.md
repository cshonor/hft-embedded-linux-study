# 条款 26：尽可能延后变量定义

## 本节讲什么

缩小变量作用域，减少不必要构造/析构，只在真正要用的时候再定义。

## 示例

```cpp
std::string process() {
    if (condition)
        std::string s;  // 需要时才定义，缩小作用域
        // ...
    return result;
}
```
