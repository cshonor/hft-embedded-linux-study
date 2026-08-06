# 条款 17：用独立语句把 new 出来的对象存入智能指针

## 本节讲什么

**Store newed objects in smart pointers in standalone statements.** 即使用了 RAII（[条款 13](./item13-以对象管理资源（RAII）.md)），把 **`new` 嵌在函数实参里** 仍可能泄漏：`new` 已完成，但对象**尚未**进入智能指针时，**其它实参求值抛异常** → 裸指针丢失。

← [条款 16：new/delete 形式](./item16-new和delete成对使用，形式保持一致.md) · 第三章收尾

---

## 危险写法

```cpp
void processWidget(std::shared_ptr<Widget> pw, int priority);

processWidget(std::shared_ptr<Widget>(new Widget), priority());
//              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^   ^^^^^^^^^^^
//              实参1的一部分                         实参2
```

看起来「`new` 已经包在 `shared_ptr` 构造里」，但 **整条函数调用** 在进 `processWidget` 之前，编译器要先准备**所有实参**。

### 编译器必须完成的三步（同一实参列表内）

1. 执行 `new Widget`  
2. 调用 `priority()`（第二个实参）  
3. 调用 `shared_ptr<Widget>` 构造函数（接管步骤 1 的指针）

与 Java/C# **固定从左到右** 求值不同，**C++ 不保证** 各实参的求值顺序（仅保证某些子表达式顺序，如 `new` 在 `shared_ptr`  ctor 之前）。

**可能顺序：**

1. `new Widget`  
2. **`priority()`** ← 若此处 **抛异常**  
3. `shared_ptr` 构造（**未执行**）

→ 步骤 1 的 `Widget*` **从未**被智能指针持有 → **泄漏**。

```
new Widget  ──→  [裸指针悬空]  ── priority() 抛异常 ──→ 泄漏
                      ↑
                 shared_ptr 还没构造
```

---

## 安全写法：独立语句

```cpp
std::shared_ptr<Widget> pw(new Widget);   // 语句1：new 与 shared_ptr 构造原子绑定
processWidget(pw, priority());            // 语句2：安全
```

- 第一条语句内，**只有** `new` 与 `shared_ptr` 构造，**没有** `priority()` 能插足。  
- 第二条语句里 `pw` 已是智能指针，即使 `priority()` 抛异常，`Widget` 仍由 `pw` 管理（或已随上一语句结束析构）。

现代更优：

```cpp
auto pw = std::make_shared<Widget>();     // 无裸 new；常单次分配
processWidget(pw, priority());
```

`make_shared` 把分配与控制块合并，仍建议 **独立语句** 再传入多实参函数（习惯一致、可读性更好）。

---

## 同样适用于 `unique_ptr`

```cpp
void f(std::unique_ptr<Widget> pw, int pri);

// ❌ 风险同 shared_ptr
f(std::unique_ptr<Widget>(new Widget), priority());

// ✅
auto pw = std::make_unique<Widget>();
f(std::move(pw), priority());
```

---

## 扩展陷阱：同一表达式里两个 `new` + 两个 `shared_ptr`

```cpp
processWidget(std::shared_ptr<Widget>(new Widget),
              std::shared_ptr<Widget>(new Widget));
```

若两个 `new` 都成功，第一个 `shared_ptr` 构造**抛异常**（少见但可能），第二个 `new` 的指针也可能泄漏。  
**对策**：每个 `new` 各自 **独立语句** + `make_shared`，或先建局部变量再传入。

---

## 与相关条款

| 条款 | 关系 |
|------|------|
| **13** | RAII 前提：`new` 必须**尽快**进管理对象 |
| **16** | 独立语句里仍要用 **`new`↔`delete` / `new[]`↔`delete[]`** 一致 |
| **15** | 进智能指针后，对外 C API 用 `.get()` |

---

## HFT 视角

- 工厂 **`createOrder()`** 返回智能指针：在工厂内 **`make_unique` 单语句完成**，不要把 `new Order` 写在调用 **`submit(processWidget(sp(new Order), prio()))`** 式嵌套实参里。  
- 热路径若避免 `make_shared` 分配策略，仍保持 **`auto p = std::unique_ptr<T>(new T(...));`** 一行独占，再 **`foo(std::move(p), ...)`**。  
- Code review：**禁止** `f(shared_ptr<T>(new T), g())` 模式。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| `f(shared_ptr(new T), other())` | 拆成两行；优先 `make_shared` |
| 以为「包了 shared_ptr 就不会漏」 | 实参求值顺序可插入 `other()` |
| 一行里两个 `new` + 两个智能指针 | 各用独立语句 |
| 临时嵌套在更复杂宏/模板里 | 先赋给局部 `shared_ptr` |

---

## 核心总结

1. **`new` 与智能指针构造** 必须在**同一独立语句**中完成，中间**不能**夹其它可能抛异常的求值。  
2. C++ **不保证** 函数实参求值顺序 → `priority()` 可能插在 `new` 与 `shared_ptr` 之间。  
3. **修复**：`auto pw = std::make_shared<T>();` 然后 `f(pw, ...)`。  
4. 优先 **`make_shared` / `make_unique`**，少写裸 `new`。  
5. 这是 [条款 13](./item13-以对象管理资源（RAII）.md) 在**语法细节**上的必要补充：**RAII 生效时间点** 不能晚于、也不能被其它实参打断。

← [条款 16：new/delete 形式](./item16-new和delete成对使用，形式保持一致.md) | [第四章：条款 18 接口设计 →](../ch04-design-declaration/item18-接口设计要易用正确、难被误用.md)
