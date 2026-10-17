# 第三章 资源管理

共 5 条条款。

## 条款

- [条款 13：以对象管理资源（RAII）](./item13-以对象管理资源（RAII）.md)
- [条款 14：资源管理类谨慎设计拷贝行为](./item14-资源管理类谨慎设计拷贝行为.md)
- [条款 15：资源管理类提供原始资源访问接口](./item15-资源管理类提供原始资源访问接口.md)
- [条款 16：new 和 delete 成对使用，形式保持一致](./item16-new和delete成对使用，形式保持一致.md)
- [条款 17：用独立语句把 new 出来的对象存入智能指针](./item17-用独立语句把new出来的对象存入智能指针.md)


## 章节摘要

资源管理：以对象管理资源（RAII）、资源管理类的拷贝行为、提供原始资源访问、`new`/`delete` 形式一致、独立语句存入智能指针。

## 代码自测

### Q1: RAII 基本模式

```cpp
class FileGuard {
    FILE *fp;
public:
    explicit FileGuard(FILE *f) : fp(f) {}
    ~FileGuard() { if (fp) fclose(fp); }
    FILE *get() const { return fp; }
};
void process() {
    FileGuard fg(fopen("data.txt", "r"));
    // ... 使用 fg.get() ...
    // 如果这里抛异常，文件会关闭吗？
}
```

> 异常抛出后文件会关闭吗？这种模式叫什么？

<details>
<summary>答案与复习指引</summary>

**会关闭。** `fg` 是栈对象，异常抛出时栈展开自动调用 `~FileGuard()` → `fclose(fp)`。这叫 **RAII**（Resource Acquisition Is Initialization）——资源获取即初始化，析构即释放。

**和 C 的区别：** C 需要手动 `fclose` 或 `goto cleanup`，异常/多返回路径容易遗漏。C++ RAII 把资源绑定到对象生命周期，自动管理。

**智能指针是 RAII 的标准实现：** `unique_ptr`/`shared_ptr` 管理动态内存，自定义删除器可管理任意资源。

**复习：** → [条款 13：以对象管理资源（RAII）](./item13-以对象管理资源（RAII）.md)
</details>

### Q2: new[]/delete 不匹配

```cpp
std::string *names = new std::string[100];
// ... 使用 ...
delete names;   // 正确吗？
```

> `delete names` 正确吗？应该用什么？

<details>
<summary>答案与复习指引</summary>

**错误——UB。** `new[]` 分配的数组必须用 `delete[]` 释放。`delete`（不带 `[]`）只调用第一个元素的析构，其余 99 个 `string` 不析构 → 内存泄漏。

**正确：** `delete[] names;` — `delete[]` 读取数组头部的元素数，逐个调用析构。

**规则：** `new` 配 `delete`，`new[]` 配 `delete[]`，typedef 别名不改变配对规则。

**现代 C++ 替代：** `std::vector<std::string>` 完全避免手动 `new[]`/`delete[]`。

**复习：** → [条款 16：new 和 delete 成对使用，形式保持一致](./item16-new和delete成对使用，形式保持一致.md)
</details>

### Q3: 独立语句存入智能指针

```cpp
// A: 危险写法
process(std::shared_ptr<Widget>(new Widget), priority());
// B: 安全写法
std::shared_ptr<Widget> pw(new Widget);
process(pw, priority());
```

> A 行有什么风险？编译器的求值顺序如何导致问题？

<details>
<summary>答案与复习指引</summary>

**A 行风险：** C++ 标准允许编译器以任意顺序求值函数参数。可能顺序：① `new Widget` ② `priority()` ③ 构造 `shared_ptr`。如果 `priority()` 在 ① 之后 ③ 之前抛异常，`new Widget` 返回的裸指针泄漏（还没被 `shared_ptr` 接管）。

**B 行安全：** `shared_ptr` 在独立语句中构造，`priority()` 在另一语句中调用。即使 `priority()` 抛异常，`pw` 已构造，RAII 自动释放。

**`make_shared` 更好：** `process(std::make_shared<Widget>(), priority())` — 一次调用完成分配+构造，无泄漏风险。

**复习：** → [条款 17：用独立语句把 new 出来的对象存入智能指针](./item17-用独立语句把new出来的对象存入智能指针.md)
</details>
