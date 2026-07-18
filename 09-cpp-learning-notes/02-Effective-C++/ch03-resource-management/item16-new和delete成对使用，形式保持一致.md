# 条款 16：new 和 delete 成对使用，形式保持一致

## 本节讲什么

**Use the same form of new and delete.** `new` 做两件事：**分配内存** + **调用构造**；`delete` 做两件事：**调用析构** + **释放内存**。`delete` 要析构几次，取决于指针指向 **单个对象** 还是 **数组** —— 由你是否写 **`[]`** 告诉编译器。形式错配 → **未定义行为（UB）**。

← [条款 15：裸资源访问](./item15-资源管理类提供原始资源访问接口.md) · 优先 RAII：[条款 13](./item13-以对象管理资源（RAII）.md)

---

## 铁律（死记）

| `new` | 必须配对的 `delete` |
|-------|---------------------|
| `new T` | `delete p` |
| `new T[n]` | `delete[] p` |

**多一个或少一个 `[]` 都是 UB**，不一定立刻崩溃，可能静默泄漏或乱析构。

---

## 1. 内存布局：单对象 vs 数组

编译器/运行时在实现上通常：

- **单对象**：一块 sizeof(T)（+ 对齐），无「元素个数」cookie  
- **数组 `new T[n]`**：可能在对象前存 **数组长度**，供 `delete[]` 知道调用 **n 次析构**

因此 `delete` **无法从裸指针本身推断**「这是数组还是单对象」——**完全靠你在 delete 时写不写 `[]`**。

```
new Widget        →  [ Widget ]
new Widget[100]   →  [ cookie: 100 | Widget | Widget | ... ]
```

（具体 cookie 布局实现定义；错配时 `delete[]` 可能把 cookie 当长度乱析构。）

---

## 2. 错配的后果

### 少写 `[]`：`new[]` + `delete`

```cpp
std::string* pal = new std::string[100];
delete pal;                    // ❌ 应是 delete[] pal
```

- **UB**  
- 典型表现：只析构 **第 1 个** `string`，其余 99 个内部缓冲 **泄漏**

### 多写 `[]`：`new` + `delete[]`

```cpp
std::string* ps = new std::string;
delete[] ps;                   // ❌ 应是 delete ps
```

- **UB**  
- `delete[]` 把对象前的内存 **误读为数组长度**，对非数组内存 **多次调用析构** → 堆损坏

---

## 3. 示例对照

```cpp
// ✅ 单对象
Widget* pw = new Widget;
delete pw;

// ✅ 数组
Widget* pa = new Widget[10];
delete[] pa;

// ❌ 混用 — 全部 UB
// delete pa;
// delete[] pw;
```

现代 C++：尽量不用裸 `new`/`delete`；若用智能指针：

```cpp
std::unique_ptr<Widget> pw(new Widget);           // delete
std::unique_ptr<Widget[]> pa(new Widget[10]);     // delete[]

std::vector<Widget> v(10);                        // ✅ 首选
```

[条款 13](./item13-以对象管理资源（RAII）.md)：**默认 `shared_ptr` / `unique_ptr` 的 deleter 是 `delete`，不能管 `new[]`** → 动态数组用 **`vector`** / **`unique_ptr<T[]>`**。

---

## 4. 类作者：多个构造函数，一个析构

类里有 **`new[]` 成员** 时，**所有构造函数** 必须用 **同一种分配方式** 初始化该指针；析构函数 **只有一个**，只能写 **一种 `delete` 形式**。

```cpp
class WebBrowser {
    char* url;   // 若有时 new char[n] 有时 new char — 析构无法统一
public:
    WebBrowser(const char* standardUrl);
    WebBrowser(const char* custom, int len);
    ~WebBrowser() { delete[] url; }   // 假定始终是数组？还是单块？
};
```

**规范**：要么 **始终** `new[]` + `delete[]`，要么 **始终** `new` + `delete`，或改用 **`std::string` / `vector<char>`** 消灭裸指针。

---

## 5. 陷阱：`typedef` 隐藏数组类型

```cpp
typedef std::string AddressLines[4];

AddressLines* pal = new AddressLines;   // 等价 new std::string[4]
delete pal;                             // ❌ 看起来像 delete 单指针，应是 delete[] pal
```

- `new AddressLines` 得到的是 **数组指针**，释放必须 **`delete[]`** —— 与视觉直觉相反。  
- **建议**：不要对数组类型 `typedef`；用 **`std::array<std::string, 4>`**、**`vector<std::string>`**、**`std::string lines[4]`**（栈上）。

---

## 与 RAII / 智能指针

| 分配 | 释放 | 智能指针 |
|------|------|----------|
| `new T` | `delete` | `unique_ptr<T>`、`shared_ptr<T>` |
| `new T[n]` | `delete[]` | `unique_ptr<T[]>`（C++11） |
| — | — | **`vector<T>`** 替代大多数 `new[]` |

自定义 **`operator new[]` / `operator delete[]`** 的类（少见）更要保证成对、形式一致。

---

## HFT 视角

- **固定上限缓冲**（如 4 行地址、N 档深度）：用 **`std::array` / `vector`**，避免 `new[]` + 手写 `delete[]` 在异常路径漏释。  
- **热路径** 若必须堆数组：`unique_ptr<T[]>` 或 **栈上数组**（维度编译期常量）。  
- Code review 重点：**任何 `delete` 向上追 `new` 是否带 `[]`**；typedef 数组类型一律警惕。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| `new[]` + `delete` | 改 `delete[]` 或改 `vector` |
| `new` + `delete[]` | 改 `delete` |
| `typedef T[N]` + `new` | 释放用 `delete[]`；更好不用 typedef |
| 多 ctor 不同 new 形式 | 统一分配策略或 RAII 成员 |
| `shared_ptr` + `new[]` | `vector` 或 `unique_ptr<T[]>` |

---

## 核心总结

1. `new` / `delete` 各两件事：分配/构造、析构/释放。  
2. **`[]` 告诉编译器「这是数组」** —— 决定析构次数。  
3. **形式必须一致**：`new`↔`delete`，`new[]`↔`delete[]`；否则 **UB**。  
4. 类设计：**一个析构** → **一种 delete**；所有 ctor 分配方式一致。  
5. **避免数组 typedef**；优先 **`vector` / `string` / `array`**。

← [条款 15：裸资源访问](./item15-资源管理类提供原始资源访问接口.md) | [条款 17：new 与智能指针 →](./item17-用独立语句把new出来的对象存入智能指针.md)
