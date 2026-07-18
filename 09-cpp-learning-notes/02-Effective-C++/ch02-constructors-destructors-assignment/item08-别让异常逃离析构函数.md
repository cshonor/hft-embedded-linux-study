# 条款 8：别让异常逃离析构函数

## 本节讲什么

**Prevent exceptions from leaving destructors.** C++ **不禁止**析构函数里抛异常，但**坚决阻止异常逃出析构函数**——若在销毁容器（如 `std::vector`）时，前一个元素的析构已抛异常，后续元素析构再抛 → **两个同时活跃的异常** → 程序 **`std::terminate`** 或未定义行为。

← [条款 7：virtual 析构](./item07-多态基类声明virtual析构函数.md) · 资源释放设计见 [条款 13](../ch03-resource-management/item13-资源管理类在构造时获取资源在析构时释放资源.md)

---

## 为什么析构不能向外抛异常？

```cpp
std::vector<Data> v;   // v 离开作用域时逐个 ~Data()
```

若第 `i` 个元素析构抛异常，运行时仍须析构 `i+1 … n-1`。若此时又一个析构抛异常 → **双异常** → C++ 无法安全处理。

**C++11 起**：析构函数默认 **`noexcept(true)`**（除非基类析构可能抛）。析构里**真正抛出**异常 → 通常直接 **`std::terminate()`**。

| 情况 | 典型结果 |
|------|----------|
| 析构抛异常，无其它活跃异常 | 仍极不推荐；可能 terminate |
| 栈展开中另一析构再抛 | **双异常 → terminate / UB** |

**结论：析构函数绝对不应该让异常传播到函数外。**

---

## 析构里必须做「可能失败」的操作怎么办？

例如资源管理类在析构里 **关闭 DB 连接 / 文件 / socket**。三种策略（作者优先级：**3 > 1 > 2**）：

### 策略 1：捕获后终止程序（Terminate）

```cpp
DBConn::~DBConn() {
    try {
        if (db.isOpen()) db.close();   // 可能抛
    } catch (...) {
        std::abort();                  // 或 std::terminate 路径
    }
}
```

- **优点**：在异常扩散、引发 UB **之前**主动结束，状态明确。
- **缺点**：整个进程被杀；客户无法恢复。

### 策略 2：捕获后吞下（Swallow）

```cpp
DBConn::~DBConn() {
    try {
        if (db.isOpen()) db.close();
    } catch (...) {
        // 记录日志；不 rethrow
    }
}
```

- **优点**：避免 terminate；程序可能继续跑。
- **缺点**：**隐瞒失败**；连接可能仍「半开」，后续行为不可靠。
- 仅当「此失败后进程仍可安全运行」且无法 redesign 时考虑。

### 策略 3：重新设计接口（最佳）

把**可能抛**的操作交给**普通成员函数**，让客户在析构**之前**处理异常；析构只做**后备（backup）**：

```cpp
class DBConn {
public:
    void close() {                     // 客户主动调用
        db.close();                    // 异常可向上传播，客户能 catch
        closed = true;
    }
    ~DBConn() {
        if (!closed) {
            try { close(); }           // 后备：客户忘了 close
            catch (...) { /* 策略 1 或 2 */ }
        }
    }
private:
    DBConnection db;
    bool closed = false;
};
```

| 角色 | 谁处理异常 |
|------|------------|
| **`close()`** | **客户** — `try/catch`、重试、告警 |
| **`~DBConn()`** | 仅当客户**忘记** `close()` 时自动尝试；失败则 abort/吞，**不再向外抛** |

这不是推卸责任，而是**给客户一个应付错误的时机**；析构是最后一道保险。

---

## 现代 C++ 补充

- 析构声明 **`noexcept`** 可文档化「此处不抛」；违反则 terminate。
- 移动/交换后析构常应 **noexcept**，否则容器强异常保证会退化为拷贝（如 `vector` reallocate）。
- **`std::uncaught_exceptions()`**（C++17）可在析构里区分「正常销毁 vs 栈展开中销毁」，高级 RAII 有时用来决定 swallow 还是 rethrow（仍须谨慎，一般析构不抛）。

```cpp
~Guard() noexcept {
    try { cleanup(); } catch (...) { log(); }
}
```

---

## HFT 视角

- **会话 / 连接 teardown**：热路径外的 `disconnect()` 由客户显式调用并处理失败；析构里只做兜底 + 日志，**不要**让断开失败异常冒到 `vector` 析构链。
- **订单批处理结束**：容器析构顺序不确定时，更忌成员析构抛异常 → 整进程 terminate，丢 in-flight 状态。
- **策略 3** 与交易「先 flat 再 shutdown」一致：业务层 `close()`，RAII 保证不泄漏。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| 析构里 `close()` 直接 rethrow | 内部 catch，abort 或 log+吞 |
| 只有析构做释放，客户无法 catch | 提供 `close()` / `release()` |
| 以为「析构抛一次没关系」 | 容器/栈展开时可能是**第二个**异常 |
| 移动析构可能抛 | 尽量 **noexcept**，保 STL 强保证 |

---

## 核心总结

1. **异常不能离开析构函数** —— 双异常 → terminate / UB。
2. 不得不在析构里调可能失败的操作 → 内部 **try/catch**，**abort** 或 **吞+日志**。
3. **最佳**：**非析构接口**（`close()`）让客户处理；析构仅**后备**，且后备失败仍不向外抛。
4. 客户需要对错误回应 → **常规成员函数**，不是析构。

← [条款 7：virtual 析构](./item07-多态基类声明virtual析构函数.md) | [条款 9：构造/析构与虚函数 →](./item09-绝不在构造和析构过程调用虚函数.md)
