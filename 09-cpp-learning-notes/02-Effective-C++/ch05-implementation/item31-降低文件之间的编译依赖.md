# 条款 31：降低文件之间的编译依赖

## 本节讲什么

依赖头文件前置声明，用指针/引用代替完整类定义；PIMPL 指针封装实现，头文件只留接口。

**有源前置声明**：`.h` 里 `class Widget;` 与「实现在已链入的 `Widget.cpp` / 库中」配套——不是空占位，而是编译期减依赖、链接期有实现（HFT 大项目常用）。

## 示例

```cpp
// 头文件尽量少 #include，多用前置声明
class Observer;
class Subject {
    std::vector<std::unique_ptr<Observer>> obs;
};
```
