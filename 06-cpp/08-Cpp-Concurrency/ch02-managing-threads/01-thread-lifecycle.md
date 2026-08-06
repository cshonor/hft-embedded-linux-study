# 2.1 std::thread 生命周期

> 第 2 章 管理线程 · 下一节：[2.2 传参](02-passing-args.md)

## 这节讲什么

`std::thread` 的创建、`join`/`detach` 的区别，以及析构时仍 joinable 会 `terminate` 的致命坑。

---

## 核心操作

```cpp
std::thread t(func);   // 创建即开始运行
t.join();              // 等待完成（阻塞）
// 或 t.detach();       // 分离，后台运行（失去句柄）
```

- **join**：阻塞调用线程直到 `t` 完成
- **detach**：分离，`t` 在后台独立运行
- **析构时仍 joinable**（未 join/detach）→ **`std::terminate`** → 程序崩溃

---

## 新手要点（和 C 的区别）

- **C 用 pthread**：`pthread_create` + `pthread_join`。C++ 的 `std::thread` 更安全（析构检查 joinable），但仍有坑。
- **最常踩的坑**：线程析构时忘了 join/detach → `terminate` 拉崩进程。每创建一个 thread，确保在所有路径（含异常）都 join 或 detach。

---

## HFT 关联

- **固定线程 + 绑核**：HFT 不在热路径创建/销毁线程（开销 + 抖动），启动时建固定线程池 + `pthread_setaffinity` 绑核。

---

## 自测题

1. `std::thread` 析构时仍 joinable 会怎样？
2. `join` 和 `detach` 的区别是什么？
3. HFT 为什么不在热路径创建线程？

---

## 参考与延伸

- 下一节：[2.2 传参](02-passing-args.md)
- 回到：[第 2 章 管理线程](README.md)
