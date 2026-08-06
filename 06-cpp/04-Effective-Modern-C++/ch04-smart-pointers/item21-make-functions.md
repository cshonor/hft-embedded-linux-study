# Item 21：优先 make_unique / make_shared 而非 new

> 第 4 章 智能指针 · Item 21 · 上一节：[Item 20 weak_ptr](item20-weak-ptr.md)

## 这节讲什么

`make_shared` 一次分配同时建对象 + 控制块，比 `shared_ptr<T>(new T)` 的两次分配更省且 cache 友好。但有些场景不能用 `make`。

---

## make 的优势

```cpp
auto sp = std::make_shared<Widget>(args);  // 1 次分配（对象 + 控制块）
auto sp = std::shared_ptr<Widget>(new Widget(args));  // 2 次分配
```

1. **单次堆分配**：对象和控制块在同一块内存，cache 友好。
2. **异常安全**：`func(shared_ptr<T>(new T), may_throw())` 中 `new T` 和 `may_throw()` 的求值顺序未指定——可能先 `new` 再抛异常，泄漏。`make_shared` 无此问题。

---

## 不能用 make 的场景

1. **自定义删除器** → 只能 `new`
2. **自定义分配器**
3. **大对象 + 长期 weak_ptr**：`make_shared` 把对象内存与控制块绑在一起——对象析构后内存延迟到所有 `weak_ptr` 销毁才回收。大对象 + 长期 `weak_ptr` 场景要用 `new`。

---

## 新手要点（和 C 的区别）

- **C 用 malloc/free**：C 程序员习惯手动分配释放。C++ 的 `make_unique`/`make_shared` 是"分配 + 构造 + 包装"一步到位的安全工厂函数。
- **规则**：优先 `make_unique`/`make_shared`，需要自定义删除器或大对象+长期 weak_ptr 时才用 `new`。

---

## HFT 关联

- **cache 友好**：`make_shared` 的单次分配让对象和控制块在同一 cache 行，减少 cache miss。

---

## 自测题

1. `make_shared` 比 `shared_ptr<T>(new T)` 省在哪里？
2. `func(shared_ptr<T>(new T), may_throw())` 有什么异常安全问题？`make_shared` 如何解决？
3. 什么场景下不能用 `make_shared` 而要用 `new`？
4. 为什么大对象 + 长期 `weak_ptr` 不适合 `make_shared`？

---

## 参考与延伸

- 下一节：[Item 22 Pimpl](item22-pimpl.md)
- 回到：[第 4 章 智能指针](README.md)
