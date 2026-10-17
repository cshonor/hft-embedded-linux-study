# C++17 移除的特性

## 被移除的特性（不再存在）

| 特性 | C++ 版本 | 替代 |
|------|---------|------|
| `std::auto_ptr` | C++11 弃用→C++17 移除 | `std::unique_ptr` |
| `std::register` 关键字 | C++11 弃用→C++17 移除 | 无（编译器早已忽略） |
| 三字符组 trigraphs | C++17 移除 | 无 |
| `std::bind1st`/`bind2nd` | C++11 弃用→C++17 移除 | `std::bind`/lambda |
| `std::unexpected` | C++11 弃用→C++17 移除 | 无（异常规范移除） |
| `bool` 的 `++` 操作 | C++17 移除 | 无 |
| `throw(type_list)` | C++11 弃用→C++17 移除 | `noexcept` |
| `std::iterator` 基类 | C++17 弃用→C++20 移除 | 直接定义 typedef |

## auto_ptr 的问题

```cpp
// auto_ptr：拷贝语义是转移所有权（反直觉）
std::auto_ptr<int> a(new int(42));
std::auto_ptr<int> b = a;  // 拷贝？不，是转移！
// a 现在为空，b 持有指针

// 在容器中灾难性：
std::vector<std::auto_ptr<int>> v;
v.push_back(std::auto_ptr<int>(new int(1)));
v.push_back(std::auto_ptr<int>(new int(2)));
// 内部拷贝可能导致悬空指针

// unique_ptr：移动语义明确
std::unique_ptr<int> a = std::make_unique<int>(42);
// auto b = a;  // ❌ 编译错误（不可拷贝）
auto b = std::move(a);  // ✅ 显式移动
```

## trigraph 移除

```cpp
// 三字符组：为没有 # 等字符的键盘设计（古老 IBM 终端）
??=   →  #
??(   →  [
??)   →  ]

// C++17 移除：
// ??=include <iostream>  // C++17 前合法（= #include）
// C++17 编译错误
```

## throw() 动态异常规范

```cpp
// C++03：动态异常规范
void foo() throw(std::bad_alloc);  // 只允许抛 bad_alloc
void bar() throw();                 // 不抛异常

// C++11：noexcept 替代
void foo() noexcept;  // 不抛异常

// C++17：throw() 完全移除
// void foo() throw(std::bad_alloc);  // ❌ 编译错误
void foo() noexcept;  // ✅
```

## 迁移检查

```bash
# GCC/Clang 警告
-Wdeprecated-declarations  # 弃用警告
-Werror=deprecated-declarations  # 弃用变错误

# 搜索老代码
grep -r "auto_ptr" src/
grep -r "register " src/  # 注意：register 作为变量名不算
grep -r "throw(" src/
grep -r "??=" src/  # trigraph
```

## 自测题

1. C++17 移除了哪些特性？列出至少 5 个。
2. `auto_ptr` 的拷贝语义有什么问题？`unique_ptr` 如何改进？
3. trigraph 是什么？为什么移除？
4. `throw()` 和 `noexcept` 的区别？为什么 `throw()` 被移除？
5. 迁移 C++17 时如何检测代码中的老旧特性？

<details>
<summary>参考答案</summary>

1. C++17 正式移除的特性（举 5 个以上）：
   1. **trigraph（三字符组）**：`??=` / `??(` / `??)` 等。
   2. **`register` 关键字**（作为存储类说明符；关键字被保留但不可再用作存储类）。
   3. **动态异常规范 `throw(T1, T2, ...)`**（带类型列表的形式）。
   4. **`std::auto_ptr`**（以及 `auto_ptr_ref`）。
   5. **`std::random_shuffle`**。
   6. **`std::unary_function` / `std::binary_function`**。
   7. **`std::ptr_fun` / `std::mem_fun` / `bind1st` / `bind2nd`** 一族。
   8. **`bool` 的自增 `++`**。
   9. `std::set_unexpected` / `std::get_unexpected` / `std::unexpected_handler` / `std::unexpected`（随动态异常规范一起移除）。
2. `auto_ptr` 的拷贝是**所有权转移**：拷贝构造/赋值会把源对象的指针**置空**。这违反了对"拷贝"的直觉——拷贝后原对象变为空，把它放进容器后 `sort`/拷贝容器会静默地把元素清空，是经典 bug 来源。
`std::unique_ptr` 的改进：**删除拷贝、只提供移动**，想转移必须显式写 `std::move`，编译期就禁止了意外的隐式转移；另外支持自定义 deleter、有数组特化 `unique_ptr<T[]>`、能安全放进容器（可移动）、语义与 `noexcept` 移动一致。
3. **trigraph（三字符组）**是 C/C++ 早期为"键盘或字符集缺少某些字符"设计的转义：翻译第一阶段把 `??=` 换成 `#`、`??(` 换成 `[`、??) 换成 `]`、??! 换成 `|` 等（源自 ISO 646 / IBM 终端时代）。
移除原因：现代字符集早已不需要它；而它会造成**意外替换**（如 `"What??!"` 里的 `??!` 被当成 `|`、字符串里出现 `??/` 触发行拼接），既增加实现负担也增加阅读负担。
4. `throw(T1, T2)` 是**动态异常规范**：声明"只允许抛这些类型"，运行期检查不匹配就调 `std::unexpected` → `terminate`，有运行期开销，且它虽是函数类型的一部分却有不少反直觉的规则（与函数指针、模板、虚函数协变交互复杂）。
`noexcept` 是**编译期**声明，不做运行期类型检查，违反时直接 `terminate`；无运行期开销，还能让优化器与标准库（`vector` 的移动、容器重分配策略）做出更好决策。
移除原因：动态异常规范几乎没有实用价值、语义复杂、有开销，实践中被广泛弃用，`noexcept` 完全覆盖了真实需求。
注意：C++17 移除的是**带类型列表**的形式；空的 `throw()` 保留为 `noexcept(true)` 的**弃用**同义词（笔记里"完全移除"的说法不准确）。
5. 分三层推进：
   - **已移除特性**直接靠编译器：trigraph、`register`、`throw(T)`、已删除的库名字会在编译时报错，全量编译一遍就能捞出来。
   - **已弃用特性**靠警告：GCC/Clang 开 `-Wdeprecated-declarations`，MSVC 对应 C4996；进一步用 `-Werror=deprecated-declarations` 把警告变错误。
   - **源码扫描**：用 grep/静态分析搜关键字（`auto_ptr`、`random_shuffle`、`bind1st`、`result_of`、`std::iterator`、`throw(`、`??=`），配合 CI 阻止新代码引入。
推荐流程：先只开警告摸清规模 → 用 `[[deprecated("...")]]` 标记内部旧 API → 分批替换 → 最后在 CI 打开 `-Werror=deprecated-declarations` 固化成果。

</details>
