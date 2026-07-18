# 条款 41：理解隐式接口和编译期多态

## 本节讲什么

模板靠表达式合法与否判定接口（隐式），运行期虚函数是显式接口 + 运行期多态。

## 示例

```cpp
template<typename Iter>
void doAdvance(Iter &it, int n) {
    it += n;  // 隐式接口：Iter 必须支持 +=
}
```
