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

---

## 代码自测

**题目 1：** `set_new_handler` 的作用是什么？
```cpp
void outOfMemory() {
    std::cerr << "Out of memory!";
    std::abort();
}
int main() {
    std::set_new_handler(outOfMemory);
    // 之后 new 失败会调用 outOfMemory
}
```

<details>
<summary>参考答案</summary>

`set_new_handler` 注册一个在 `operator new` 无法满足内存请求时调用的回调函数。回调可以：1) 释放可用内存后返回（让 new 重试）；2) 抛 `bad_alloc` 异常；3) 调用 `abort()`/`exit()` 终止程序。不设 handler 时默认抛 `bad_alloc`。

</details>
