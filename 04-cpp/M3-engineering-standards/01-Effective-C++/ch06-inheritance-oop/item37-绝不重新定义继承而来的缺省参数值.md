# 条款 37：绝不重新定义继承而来的缺省参数值

## 本节讲什么

编译期绑定参数，运行期绑定函数，极易出现逻辑诡异 bug。

## 示例

```cpp
class Base { public: virtual void print(int x = 10) = 0; };
class Derived : public Base {
public:
    void print(int x = 20) override;  // 不要改缺省参数
};
```
