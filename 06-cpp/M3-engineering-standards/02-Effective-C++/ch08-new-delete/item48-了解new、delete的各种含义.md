# 条款 48：了解 new、delete 的各种含义

## 本节讲什么

普通 `operator new`、`placement new`、`operator new[]`、全局/类内重载版本区分开。

## 示例

```cpp
int *p = new int;
delete p;
Widget *w = ::new Widget;
::delete w;
void *buf = ::operator new(64);
::operator delete(buf);
```
