# 条款 12：抛出异常时，理解对象拷贝的完整流程与开销

## 本节讲什么

**Understand how throwing an exception differs from passing a parameter or calling a virtual function.**
`throw obj;` 不是"把 obj 送出去"——异常对象会**被拷贝**到一个特殊的
异常存储区（exception object），拷贝用**静态类型**的拷贝构造函数。
这决定了：按值捕获会切片、按引用捕获才是正道、throw 的对象寿命与作用域无关。

← 上一条 [item11 资源管理函数](./item11-杜绝资源管理函数内部发生异常造成泄漏.md)；
下一条 [item13 catch(...) 兜底](./item13-合理使用catch(...)全局捕获，谨慎兜底.md)。

---

## 1. throw 的完整流程（与函数调用三个本质区别）

```cpp
void f() {
    DerivedError e;
    throw e;        // ① 用**静态类型**（DerivedError）的拷贝构造，
                    //    把 e 拷到异常存储区（exception object）
                    // ② e 本身随栈展开析构（正常局部对象）
                    // ③ catch 拿到的是**存储区那份拷贝**
}
```

| 维度 | 函数传参 | throw |
|---|---|---|
| 拷贝构造用的类型 | 实参动态类型可保留（引用/指针参数） | **静态类型**（`throw` 表达式的声明类型） |
| 对象寿命 | 参数随调用帧生灭 | 异常对象活到**最后一个 catch 结束** |
| 拷贝次数 | 按值一次 | **可能两次**（throw 一次 + 按值 catch 又一次） |

**切片陷阱的根源**：`throw BaseError(...)` 即使实参是派生对象，
按静态类型 BaseError 拷贝——派生部分当场切掉。
（反过来：`throw DerivedError(...)` 里存的是完整派生对象，
catch 按 Base 引用拿也能保留多态——见下。）

## 2. catch 的三种形态（只有一种是对的）

```cpp
try { risky(); }
catch (Error e)        { /* ❌ 按值：又拷一次 + 若 e 是基类则切片 */ }
catch (Error* e)       { /* ❌ 按指针：只能接 throw new Error(...)——
                            谁来 delete？（item21 错误二同款泄漏） */ }
catch (const Error& e) { /* ✅ 按 const 引用：零拷贝 + 多态保留（动态类型完整） */ }
```

**按 const 引用捕获**是本条与 18.1 ③ 的共同结论：
- 零拷贝（直接用异常存储区那份）
- 多态保留（`catch (const std::exception&)` 里 `e.what()` 走虚函数，
  调到派生实现——按值捕获时 what() 已被切成基类版）
- `throw;`（空 throw）在 catch 内**原样重抛**当前异常对象（不切片）；
  `throw e;` 会按 e 的静态类型**重新拷贝**（切片源之二）

## 3. 开销解剖（为什么异常"贵"）

```
throw 的总成本 = 拷贝构造异常对象（1-2 次）
               + 查 LSDA/异常表（找到 landing pad）
               + 栈展开（每帧：局部对象析构 + 清理表执行）
               + catch 匹配（按 catch 链顺序做类型检查）
```

- 关键认知：**不抛异常时**这套机制零成本（表驱动，无异常时无运行时代码）——
  这就是"异常给冷路径"的理论基础（→ 18.1 HFT）
- 抛一次的成本在 **µs 级**（展开深度和析构复杂度决定），
  且不可预测（表的 cache miss）——热路径永远不用它做流程控制

## HFT 关联

- 冷路径错误统一基类 `class AppError : public std::runtime_error`：
  启动/配置/重连的错误按 const 引用捕获，保留完整错误信息
- `throw;` vs `throw e;` 的区别在**错误翻译层**是高频坑：
  网关层想"记一笔再往上抛"必须 `throw;`——`throw e;` 会把
  派生错误切成 `std::exception`，上层重试策略（按错误类型分派）全废
- 异常对象的拷贝成本给了"-fno-exceptions 派"一个论据：
  热路径库的错误通道用 `expected<T, ErrorCode>`（C++23）——
  值语义、零拷贝、编译期强制检查（→ 18.1 / item54 的 expected）

## 代码自测

**题目 1：** `throw` 的拷贝构造为什么用静态类型？这导致什么经典陷阱？

<details>
<summary>参考答案</summary>

`throw expr;` 的语义是"按 expr 的**静态（声明）类型**拷贝一份到异常存储区"——
编译期就确定调用哪个拷贝构造，与实参的动态类型无关。
陷阱：`Base& ref = derived_obj; throw ref;`——静态类型是 Base，
派生部分当场切片，catch 里拿到的只是 Base 残骸
（多态信息、派生字段全丢）。
修法：throw 时**显式构造**（`throw DerivedError(...)`）——
让静态类型就是你想传的完整类型。

</details>

**题目 2：** catch 块里"记完日志继续往上抛"，`throw;` 和 `throw e;` 有什么区别？

<details>
<summary>参考答案</summary>

`throw;`（空 throw）：**原样重抛**当前异常对象——
异常存储区那份原封不动继续向上传，动态类型/内容完整。
`throw e;`：按 **e 的静态类型**重新拷贝一个新异常对象——
若 e 声明为基类引用（`catch (const std::exception& e)`），
派生类型被切片：上层的 `catch (const NetworkError&)` 再也接不到，
重试策略（按具体错误类型分派）静默失效。
规则：翻译/记录层永远 `throw;`——`throw e;` 只在
"确实要构造新异常"的场景（换错误类型）才写。

</details>

**题目 3：** 为什么"不抛异常时异常机制零成本"，而热路径仍然禁用异常？

<details>
<summary>参考答案</summary>

零成本指的是**表驱动实现的静态面**：无异常时，LSDA/展开表只是
数据段里的表格，不执行任何额外指令（对比 setjmp 式实现的常驻开销）。
但热路径禁异常的理由在**动态面**：
① 一旦抛出，查表 + 逐帧展开 + 析构的成本是 µs 级且**抖动不可预测**
（表冷 cache、展开深度不定——18.1 HFT 的延迟预算容不下）；
② 异常作为**控制流**会破坏编译器对热路径的优化
（可能的抛出点阻断指令调度/寄存器分配）；
③ 团队纪律的简洁性：错误码/expected 的路径在代码里**显式可见**，
异常路径是隐式的——热路径的可审计性压倒一切。
"零成本"说的是没用到的时候，"禁用"管的是用到的时候——两者不矛盾。

</details>
