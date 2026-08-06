# 条款 15：资源管理类提供原始资源访问接口

## 本节讲什么

**Provide access to raw resources in resource-managing classes.** RAII（[条款 13](./item13-以对象管理资源（RAII）.md)）防泄漏，但大量 **C API / 旧接口** 只认**裸资源**（裸指针、`FILE*`、`FontHandle`）。把智能指针对象直接传给要 **`T*`** 的函数 → **编译失败**。因此每个 RAII 类都要提供 **取出底层裸资源** 的途径：**显式转换**（首选）或 **隐式转换**（方便但危险）。

← [条款 14：拷贝行为](./item14-资源管理类谨慎设计拷贝行为.md) · 易用接口见 [条款 18](../ch04-design-declaration/item18-接口设计要易用正确、难被误用.md)

---

## 典型场景

```cpp
Investment* createInvestment();
void daysHeld(const Investment* p);

std::shared_ptr<Investment> pInv(createInvestment());

daysHeld(pInv);              // ❌ 要 Investment*，不是 shared_ptr
daysHeld(pInv.get());        // ✅ 显式取出裸指针
```

智能指针与自定义 RAII 包装（[条款 14](./item14-资源管理类谨慎设计拷贝行为.md) 的 `Font`、`Lock`）都面临同一需求：**与裸资源世界对接**。

---

## 1. 显式转换（Explicit — 通常更安全）

### 智能指针：`get()`、`operator*`、`operator->`

```cpp
std::shared_ptr<Investment> pInv = ...;

daysHeld(pInv.get());        // 裸指针，不增减引用计数
pInv->someMember();          // operator-> → 底层对象
(*pInv).someMember();        // operator*
```

| 接口 | 作用 |
|------|------|
| **`get()`** | 返回裸指针；**不转移**所有权；**不释放** |
| **`operator->` / `operator*`** | 访问所指对象，仍由智能指针管理生命周期 |

**注意**：`get()` 返回的指针**别长期保存**并在 `shared_ptr` 销毁后使用 → 悬空。

### 自定义 RAII 类：显式 `get()`

```cpp
using FontHandle = void*;   // C API 句柄

void releaseFont(FontHandle fh);
FontHandle changeFontSize(FontHandle fh, int size);

class Font {
public:
    explicit Font(FontHandle fh) : f(fh) {}
    ~Font() { releaseFont(f); }

    FontHandle get() const { return f; }   // 显式访问

private:
    FontHandle f;
};

void f() {
    Font font(getFont());
    int days = daysHeld(font.get());           // 显式
    FontHandle h = changeFontSize(font.get(), 12);
}
```

- **优点**：转换**可见**，误用概率低（符合 [条款 18](../ch04-design-declaration/item18-接口设计要易用正确、难被误用.md)）。  
- **缺点**：每次调 C API 都要 `.get()`，略繁琐。

---

## 2. 隐式转换（Implicit — 方便但易错）

```cpp
class Font {
public:
    operator FontHandle() const { return f; }  // 隐式转成裸句柄
};

void f() {
    Font font(getFont());
    int days = daysHeld(font);                 // 隐式转换，写法自然
    changeFontSize(font, 12);
}
```

### 致命风险：无意拷贝 + 悬空句柄

```cpp
Font font(getFont());
FontHandle h = font;           // 隐式转换：h 是裸拷贝
// font 析构 → releaseFont(f)
// h 已悬空 — 若后续 API 仍用 h → UB
```

隐式转换让客户**容易**在不知情时拿到**不受 RAII 保护**的裸资源副本；RAII 对象一销毁，裸句柄 **dangle**。

| 方式 | 安全性 | 便利性 |
|------|--------|--------|
| **`get()` 显式** | 高 — 调用点可见 | 略烦 |
| **隐式 `operator T()`** | 低 — 易无意转换 | 写法短 |

**惯例**：除非 API 极固定且团队约束强，否则 **优先显式 `get()`**；C++11 起可用 **`explicit` 构造函数** 限制别处的隐式转换。

---

## 「破坏封装」不是问题

返回裸资源看似**破坏封装**，但 RAII 类的首要目的不是隐藏数据，而是 **保证资源释放**（析构 / deleter）。良好设计：

- **隐藏**客户不必关心的机制（引用计数、deleter、锁状态）；  
- **暴露**客户必须交给 C API 的**底层句柄**。

封装与裸资源访问**可以并存**。

---

## 使用裸指针时的纪律

1. **`get()` 不延长生命周期** —— 在智能指针/RAII 对象**存活期间**使用裸指针。  
2. **不要把 `get()` 结果存进长期变量** 除非你也持有 `shared_ptr`。  
3. **禁止**对 `get()` 返回指针 **`delete`**（所有权仍在 RAII 对象）。  
4. 若 C API **接管所有权**（take ownership），需 **`release()`** 式接口（如 `unique_ptr::release()`），而非裸 `get()`。

---

## HFT 视角

- **DPDK / 内核 / C 行情库**：C++ 侧 `shared_ptr<Session>` + 调 C 函数时 **`session.get()`** 或 **`handle()`** 显式传参。  
- **字体/配置类包装** 少见，但 **socket fd、MBUF 指针** 同理：热路径可缓存 `get()` 到局部 **`T*`**，**作用域内**用完，别跨异步回调裸存。  
- 隐式转换在大型工程里易埋 **use-after-free** —— 交易代码优先 **显式**。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| 把 `shared_ptr` 当 `T*` 传 | `.get()` |
| `get()` 后 `shared_ptr` 已销毁仍用裸指针 | 缩短裸指针生命周期 |
| 隐式转换拷贝句柄后对象析构 | 改显式 `get()`；别存裸句柄 |
| 对 `get()` 指针 `delete` | 所有权规则：`release` 或让 RAII 析构 |
| RAII 类无任何裸资源出口 | 补 `get()` / `operator->` |

---

## 核心总结

1. **每个 RAII 类**都应提供取得**底层裸资源**的途径 —— 否则无法对接 C API。  
2. **显式**：`get()`、`operator*`、`operator->` —— **更安全**，首选。  
3. **隐式**：`operator Handle()` —— 方便但易 **dangle**，慎用。  
4. RAII 目标是 **释放动作**，不是绝对隐藏句柄；**显式 + 条款 18** 降低误用。  
5. 使用裸资源时：**不延长生命周期、不重复 delete**。

← [条款 14：拷贝行为](./item14-资源管理类谨慎设计拷贝行为.md) | [条款 16：new/delete 成对 →](./item16-new和delete成对使用，形式保持一致.md)
