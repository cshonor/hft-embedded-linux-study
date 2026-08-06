# 3.4 初始化保护

> 第 3 章 · 上一节：[3.3 接口级竞争](03-interface-race.md) · 下一节：[3.5 读写锁](05-shared-mutex.md)

## 这节讲什么

`call_once` 和 Meyers singleton 保证初始化只执行一次且线程安全。C++11 起 `static` 局部变量的初始化由编译器保证线程安全。

---

## 两种方式

### call_once

```cpp
std::once_flag flag;
std::call_once(flag, []{ init(); });   // 只初始化一次，线程安全
```

### Meyers Singleton

```cpp
static Config& inst() {
    static Config c;   // C++11 起线程安全初始化
    return c;
}
```

C++11 起 `static` 局部变量的初始化由编译器保证线程安全（等价于 `call_once` 语义）——不需要手写 `double-checked locking`（DCLP）。

---

## 新手要点

- **DCLP 已过时**：C++11 前手写 `double-checked locking` 有内存序 bug。C++11 起用 Meyers singleton，编译器保证正确。
- **别用 DCLP**：新手看到老代码里的 DCLP 别学——用 `static` 局部变量替代。

---

## HFT 关联

- **配置/单例**：HFT 守护进程的全局配置/单例用 Meyers singleton，C++11 起保证线程安全初始化。

---

## 自测题

1. C++11 起 `static` 局部变量的线程安全性由谁保证？
2. 为什么 DCLP（双重检查锁）已过时？
3. `call_once` 和 Meyers singleton 有什么关系？

---

## 参考与延伸

- 下一节：[3.5 读写锁](05-shared-mutex.md)
- 回到：[第 3 章](README.md)
