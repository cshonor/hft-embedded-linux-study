# Item 25：对万能引用用 std::forward，对右值引用用 std::move

> 第 5 章 右值引用、移动语义与完美转发 · Item 25 · 上一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)

## 为什么要学这个（先建立直觉）

[Item 23](item23-move-and-forward.md) 讲了 `move`/`forward` 的本质都是 cast。这一条回答更实际的问题：**写代码时到底用哪个？**

关键认知先立住：**函数内部，形参永远是左值**——它是个有名字的变量，不管你声明的是 `Widget&&` 还是 `T&&`：

```cpp
void take(Widget&& w) {
    // w 在这里是左值（有名字、能取地址）
    store(w);            // 走拷贝重载！移动语义丢失
    store(std::move(w)); // 恢复右值性，走移动
}
```

如果你不做处理就把 `w` 往下传，移动语义在"透传"时全部丢光。所以规则只有一句：**右值引用形参用 `std::move` 恢复右值性，万能引用形参用 `std::forward<T>` 保留原始值类别**。混用——尤其是对万能引用用 `move`——会掏空调用方的左值，是编译通过、运行不崩、数据悄悄丢的事故。

---

## 这节讲什么

使用规则：右值引用形参 → `std::move`；万能引用形参 → `std::forward<T>`；多次使用时只在**最后一次**转换；返回局部对象时两个都不用（让 NRVO 生效）。

---

## 两条核心规则（实测）

用带打印的 `Sink` 观察值类别是否被正确保留，本机实测（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
struct Sink {
    std::string name;
    void assign(const std::string& s) { name = s; std::cout << "  拷贝赋值（源保留）\n"; }
    void assign(std::string&& s) { name = std::move(s); std::cout << "  移动赋值（源掏空）\n"; }
};

void takeRvalue(std::string&& s, Sink& sink) {     // 右值引用形参
    sink.assign(std::move(s));
}
template<class T>
void takeUniversal(T&& s, Sink& sink) {            // 万能引用形参
    sink.assign(std::forward<T>(s));
}
```

实测输出：

```
takeRvalue(std::string("tmp")):
  移动赋值（源掏空）                  ← 右值引用 + move：正确移动
takeUniversal(左值 lv):
  拷贝赋值（源保留）                  ← 万能引用 + forward：左值走拷贝
  lv = important（左值实参安然无恙）
takeUniversal(std::string("rv")):
  移动赋值（源掏空）                  ← 万能引用 + forward：右值走移动
```

若把 `takeUniversal` 里的 `forward` 换成 `move`，传左值 `lv` 也会走移动赋值——`lv` 被掏空（实测现场见 [Item 23](item23-move-and-forward.md) 的对照实验）。

---

## 返回值不要 std::move：NRVO 实测

新手常以为返回局部对象要 `return std::move(w);` 才"高效"。恰恰相反——这会**妨碍 NRVO**（具名返回值优化：编译器直接在调用方的接收位置构造局部对象，零拷贝零移动）。本机实测，对比构造次数：

```cpp
struct Widget {
    Widget() { std::cout << "默认构造\n"; }
    Widget(const Widget&) { std::cout << "拷贝构造\n"; }
    Widget(Widget&&) noexcept { std::cout << "移动构造\n"; }
};

Widget makeGood() { Widget w; return w; }
Widget makeBad()  { Widget w; return std::move(w); }
```

默认编译（NRVO 开启）实测输出：

```
-- return w; --
默认构造                    ← 总共 1 次构造，零拷贝零移动
-- return std::move(w); --
默认构造
移动构造                    ← 白白多了一次移动
```

clang 还直接给了警告（`-Wpessimizing-move`）：

```
warning: moving a local object in a return statement prevents copy elision
         [-Wpessimizing-move]
    return std::move(w);
           ^
note: remove std::move call here
```

再关掉拷贝省略（`-fno-elide-constructors`）实测：

```
-- return w; --
默认构造
移动构造                    ← 即使没 NRVO，return w 也按右值处理，走移动
-- return std::move(w); --
默认构造
移动构造
```

结论一目了然：`return w;` 最坏情况 = `return std::move(w);`，最好情况（NRVO）零开销。**写 `move` 只有害没有利**。这也回答了"NRVO 失败会不会退回拷贝"的担心：不会，标准规定 NRVO 不适用时局部对象按右值处理，走移动而非拷贝。

---

## 最后一次使用才 move/forward

多次使用的场景，前面的使用保持原样，最后一次才转交所有权：

```cpp
template<class T>
void useTwice(T&& s, Sink& sink) {
    std::cout << "  第一次用（只读长度）: " << s.size() << "\n";
    sink.assign(std::forward<T>(s));   // 最后一次使用才转交
}
```

实测输出：

```
useTwice(std::string("data")):
  第一次用（只读长度）: 4       ← 先读再用，顺序正确
  移动赋值（源掏空）
```

提前 move 的话，第一次读到的就是被掏空后的未指定内容。

---

## 完整可运行例子

NRVO 对比实验完整代码（clang 23.1.0, `-std=c++17 -Wall`）：

```cpp
#include <iostream>
#include <string>
#include <utility>

struct Widget {
    std::string payload;
    Widget() { std::cout << "默认构造\n"; }
    Widget(const Widget&) { std::cout << "拷贝构造\n"; }
    Widget(Widget&&) noexcept { std::cout << "移动构造\n"; }
};

Widget makeGood() {          // 直接 return 局部对象
    Widget w;
    return w;
}

Widget makeBad() {           // return std::move
    Widget w;
    return std::move(w);     // clang 警告：-Wpessimizing-move
}

int main() {
    std::cout << "-- return w; --\n";
    Widget a = makeGood();
    std::cout << "-- return std::move(w); --\n";
    Widget b = makeBad();
    (void)a; (void)b;
}
```

实测输出（默认编译，NRVO 生效）：

```
-- return w; --
默认构造
-- return std::move(w); --
默认构造
移动构造
```

---

## 常见错误（新手踩坑）

1. **万能引用用了 `move`**。`template<class T> void add(T&& x) { v.push_back(std::move(x)); }`——传左值时左值被掏空。修正：`std::forward<T>(x)`。
2. **`return std::move(local)`**。阻止 NRVO，零开销变一次移动，clang 实测直接报 `-Wpessimizing-move`。修正：`return local;`。
3. **提前 move，后面还用**。`std::move(x)` 之后的任何读取都是未指定内容。规则：最后一次使用才转换。
4. **以为右值引用形参可以直接传下去**。`void f(Widget&& w) { g(w); }`——`w` 是左值，走拷贝。必须 `g(std::move(w))`。

---

## HFT 关联

- **订单构造转发**：`template<class... Args> void emplaceOrder(Args&&... args) { book.emplace_back(std::forward<Args>(args)...); }`——完美转发构造参数，避免拷贝订单结构体。
- **回调注册**：`template<class F> void onTick(F&& cb) { handlers.push_back(std::forward<F>(cb)); }`——左值回调拷贝、右值回调移动，各得其所。
- **热路径返回值**：返回大对象（如聚合后的行情快照）时 `return result;` 让 NRVO 生效；写 `return std::move(result)` 反而多一次移动，编译器还会警告。
- **"最后一次使用"原则**在流水线代码里尤其重要：先校验后入库，`move` 永远放在校验之后。

---

## 自测题

**1. 为什么右值引用形参在函数体内是左值？**

<details><summary>答案</summary>
因为它有名字、能取地址、能被多次使用——按值类别规则，具名变量是左值表达式。`void f(Widget&& w)` 里直接把 `w` 传下去会走拷贝重载，必须 `std::move(w)` 恢复右值性。
</details>

**2. 万能引用形参为什么不能配 `std::move`？**

<details><summary>答案</summary>
万能引用接左值时 `T` 推为 `T&`，形参绑定着调用方的左值对象。`move` 无条件转右值会把调用方的左值掏空——编译通过、不崩溃、数据悄悄丢。`forward<T>` 按 `T` 的推导结果条件转换，左值进左值出、右值进右值出。
</details>

**3. `return std::move(local)` 为什么有害？实测构造次数各是多少？**

<details><summary>答案</summary>
它把返回表达式变成 `T&&`，破坏 NRVO 前提。实测：`return w;` 默认编译只有 1 次默认构造（NRVO 直接在接收位置构造）；`return std::move(w);` 是默认构造 + 移动构造。即使 `-fno-elide-constructors`，`return w;` 也按右值处理走移动，不比 `move` 差。clang 对后者报 `-Wpessimizing-move` 警告。
</details>

**4. 多次使用万能引用形参时，`forward` 应该放在哪？**

<details><summary>答案</summary>
最后一次使用。前面的使用按原样（左值）访问；提前 `forward`（实参为右值时等价 move）会把对象掏空，后续读到未指定内容。
</details>

**5. 对右值引用形参用 `std::forward` 行不行？**

<details><summary>答案</summary>
能编译且行为正确（`forward<Widget>(w)` 等价 `move`），但语义误导读者以为这是万能引用。惯例：右值引用写 `move`，万能引用写 `forward`——代码即文档。
</details>

**6. NRVO 不适用时（如关掉拷贝省略），`return w;` 会退回拷贝吗？**

<details><summary>答案</summary>
不会。标准规定：NRVO 不适用时，返回的局部对象按右值处理，选中移动构造。实测 `-fno-elide-constructors` 下 `return w;` 输出"默认构造 + 移动构造"，没有拷贝。只有连移动构造都没有时才退回拷贝（见 [Item 29](item29-move-not-exist.md)）。
</details>

**7. 下面代码有什么问题？**

```cpp
template<class T>
void store(T&& x) {
    log(x);
    cache = std::move(x);
}
```

<details><summary>答案</summary>
`x` 是万能引用却用了 `move`：传左值时 `log(x)` 读到的是原值（侥幸正确），但随后 `cache = std::move(x)` 把调用方的左值掏空。修正：`cache = std::forward<T>(x);`，且它确实是最后一次使用，位置正确。
</details>

---

## 参考与延伸

- 上一节：[Item 24 万能引用 vs 右值引用](item24-universal-vs-rvalue.md)
- 下一节：[Item 26 避免万能引用重载](item26-avoid-overloading-universal.md)
- 回到：[第 5 章 右值引用、移动语义与完美转发](README.md)
