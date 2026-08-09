# 条款 11：赋值运算符处理自我赋值

## 本节讲什么

**Handle assignment to self in operator=.** **自赋值**（`w = w`）显式少见，但 **别名** 下很常见：`*px = *py`（同一对象）、`a[i] = a[j]`（`i == j`）。管理**动态资源**的 `operator=` 若未防范 → 先 `delete` 源再拷贝 → **悬空指针**；还要兼顾 **异常安全**。

← [条款 10：返回 *this](./item10-令operator=返回this引用.md) · 复制每个成员见 [条款 12](./item12-复制对象时勿忘其每一个成员.md)

---

## 1. 自赋值陷阱：先 delete 再拷贝

```cpp
class Widget {
    Bitmap* pb;
public:
    Widget& operator=(const Widget& rhs) {   // ❌ 危险写法
        delete pb;
        pb = new Bitmap(*rhs.pb);          // 若 *this == rhs，pb 已被删
        return *this;
    }
};
```

| 步骤（自赋值时） | 后果 |
|------------------|------|
| `delete pb` | 删掉 **rhs 也在用的** 那份 Bitmap |
| `new Bitmap(*rhs.pb)` | 从**已释放**内存拷贝 → UB / 崩溃 |

**别名示例：**

```cpp
Widget w;
Widget& ra = w;
Widget& rb = w;
ra = rb;              // 自赋值

Widget* pa = &w;
Widget* pb = &w;
*pa = *pb;            // 自赋值
```

---

## 2. 方案 A：一致性检测（Identity test）

```cpp
Widget& operator=(const Widget& rhs) {
    if (this == &rhs) return *this;       // 条款 10：仍 return *this
    delete pb;
    pb = new Bitmap(*rhs.pb);
    return *this;
}
```

- ✅ 解决**显式/别名自赋值**误删源  
- ❌ **异常不安全**：若 `new Bitmap(*rhs.pb)` 抛异常（内存不足、拷贝 ctor 失败），**旧资源已 delete** → 对象损坏（指针悬空，无法回滚）

---

## 3. 方案 B：调整顺序 —— 先分配/拷贝，再释放旧资源

**异常安全**的实现往往**同时自赋值安全**，无需 `if (this == &rhs)`：

```cpp
Widget& operator=(const Widget& rhs) {
    Bitmap* pOrig = pb;                   // ① 记住旧资源
    pb = new Bitmap(*rhs.pb);             // ② 先拿到新副本（可能抛异常）
    delete pOrig;                         // ③ 成功后再删旧的
    return *this;
}
```

| 情况 | 行为 |
|------|------|
| **`new` 抛异常** | `pb` 仍指向旧 Bitmap，对象**完好**（强异常保证倾向） |
| **自赋值** | 先 `new Bitmap(*rhs.pb)` 得到**独立副本**，再 `delete` 旧 `pb` —— 正确 |
| **性能** | 自赋值也会多一次拷贝；若确信自赋值极多，可加 `if (this == &rhs)`，但多分支可能影响预取 |

```cpp
Widget& operator=(const Widget& rhs) {
    if (this == &rhs) return *this;       // 可选：省一次自拷贝
    Bitmap* pOrig = pb;
    pb = new Bitmap(*rhs.pb);
    delete pOrig;
    return *this;
}
```

---

## 4. 方案 C：Copy-and-Swap（拷贝并交换）

通用手法：**先做 rhs 的副本，再与 `*this` 交换**；交换通常 **noexcept**。

### 4.1 手动副本 + swap

```cpp
void swap(Widget& other) noexcept {
    using std::swap;
    swap(pb, other.pb);
}

Widget& operator=(const Widget& rhs) {
    Widget temp(rhs);                     // 拷贝 rhs
    swap(temp);                           // *this 与 temp 交换
    return *this;                       // temp 析构带走旧资源
}
```

### 4.2 传值 copy-and-swap（作者推荐变体）

利用 **按值传参** 在入口完成拷贝：

```cpp
Widget& operator=(Widget rhs) {           // 按值：拷贝或移动构造 rhs
    swap(rhs);                            // noexcept swap
    return *this;                       // rhs 析构旧数据
}
```

- **自赋值安全**：拷贝/移动自己再 swap，旧状态由局部对象析构  
- **异常安全**：拷贝在进函数前完成；swap 不抛  
- C++11 起同一签名可同时服务**拷贝与移动赋值**（`Widget rhs` 绑定右值时走移动）

---

## 三种方案对比

| 方案 | 自赋值 | 异常安全 | 备注 |
|------|--------|----------|------|
| 先 delete 再 new | ❌ | ❌ | 禁止 |
| **`if (this == &rhs)`** + 先删后 new | ✅ | ❌ | 仅防自赋值 |
| **先 new 后 delete** | ✅ | ✅（new 失败则完好） | 经典强保证思路 |
| **copy-and-swap** | ✅ | ✅ | 现代首选；配合 `swap` noexcept |

---

## 不止 operator=：多对象可能是同一实体

任何同时操作**两个以上**对象的函数都要考虑别名，例如：

```cpp
void swap(Widget& a, Widget& b) {
    if (&a == &b) return;                 // a、b 同一对象时 no-op
}
```

资源管理类见 [条款 13](../ch03-resource-management/item13-以对象管理资源（RAII）.md)：优先 **RAII + 智能指针**，减少手写 `delete` 的 `operator=`。

---

## HFT 视角

- **共享行情缓冲指针** 的句柄类：`*h1 = *h2` 可能自赋值；禁止「先 release 再 copy 源」。
- **热路径** 若自赋值极少，copy-and-swap 多一次分配可能贵 → 可 **先 new 后 delete** + 可选 identity test；但**正确性优先**。
- **`vector` 元素赋值**、容器 `assign` 内部已处理；**自定义资源类**才需手写条款 11。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| `delete` 后从 `rhs` 拷贝且可能 `*this == rhs` | 先副本再删旧 |
| 只有 `if (this==&rhs)`，new 仍可能抛 | 先 new 后 delete 或 copy-and-swap |
| swap 可能抛 | `swap` 标 **noexcept** |
| 忘记 `return *this` | [条款 10](./item10-令operator=返回this引用.md) |

---

## 核心总结

1. **自赋值合法且隐蔽** —— 指针/引用/下标别名。  
2. **先删后拷** 在自赋值时 = 删掉源。  
3. **三大技巧**：地址比较（必要但不充分）、**先分配后删除**、**copy-and-swap**（可传值参数）。  
4. 异常安全与自赋值安全**常一并解决**；现代代码优先 **copy-and-swap + noexcept swap**。  
5. 多对象 API 也要考虑「可能是同一个实体」。

← [条款 10：返回 *this](./item10-令operator=返回this引用.md) | [条款 12：复制每一个成员 →](./item12-复制对象时勿忘其每一个成员.md)
