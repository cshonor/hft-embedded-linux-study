# 条款 40：谨慎使用多重继承

## 本节讲什么

容易出现歧义、菱形继承、成员名冲突；能单继承 + 组合解决就不用多继承。

## 示例

```cpp
class A {};
class B : public A {};
class C : public A {};
class D : public B, public C {};  // 多重继承需谨慎
```
