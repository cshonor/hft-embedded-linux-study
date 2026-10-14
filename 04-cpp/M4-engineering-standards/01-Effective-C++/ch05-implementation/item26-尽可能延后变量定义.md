# 条款 26：尽可能延后变量定义

## 本节讲什么

**Postpone variable definitions as long as possible.** 变量定义 = 构造 + 析构的成本。
定义得早而用得晚（甚至没用上），就是为不存在的使用付费。三层处理：
**异常路径浪费 → 循环内 vs 循环外 → 默认构造的陷阱**。这条与 item04（初始化）
一体两面：不仅要初始化，还要**在正确的时机**初始化。

← 上一条 [item25 不抛异常的 swap](../ch04-designs-and-declarations/item25-考虑提供不抛异常的swap重载.md)；
下一条 [item27 减少类型转型](./item27-尽量减少类型转型（cast）.md)。

---

## 1. 核心问题：早定义 = 为可能不走的路径付费

```cpp
std::string encrypt_password(const std::string& password) {
    std::string encrypted;                 // ❌ 现在就构造（默认构造一次）

    if (password.length() < MIN_LENGTH) {
        throw std::logic_error("too short");   // ← 异常路径：encrypted 白构造白析构
    }
    // ...真正加密...
    return encrypted;
}
```

异常抛出时，`encrypted` 经历了**默认构造 + 析构**——完全浪费。
延后到确定要用的地方：

```cpp
std::string encrypt_password(const std::string& password) {
    if (password.length() < MIN_LENGTH) throw std::logic_error("too short");
    std::string encrypted(password);       // ✅ 直接用值构造，连默认构造都省了
    // ...
    return encrypted;
}
```

注意第二个收益：`std::string encrypted;` + 后续赋值 = **默认构造 + operator=**；
`std::string encrypted(password);` = **一次拷贝构造**——延后的同时往往还能
跳过默认构造（item04 的"初始化列表优于赋值"同款原理）。

## 2. 循环内 vs 循环外（Meyers 的原始分析）

```cpp
// 方案 A：循环外定义——1 次构造 + 1 次析构 + n 次赋值
Widget w;
for (int i = 0; i < n; ++i) { w = make(i); use(w); }

// 方案 B：循环内定义——n 次构造 + n 次析构
for (int i = 0; i < n; ++i) { Widget w = make(i); use(w); }
```

| 成本对比 | 选 A | 选 B |
|---|---|---|
| 赋值成本 ≤ 构造+析构 | ✅ | |
| 赋值成本 > 构造+析构（常见！） | | ✅ |
| 要求变量不带出循环（作用域卫生） | | ✅ |
| Widget 含大缓冲（复用避免反复分配） | ✅（有意的） | |

经验法则：**默认选 B**（作用域干净、无残留状态），除非①赋值明显便宜、
②或你在**有意复用**大缓冲（那就把意图写成注释/命名，如 `reuse_buf`）。

## 3. "默认构造后再赋值"的隐蔽版本

```cpp
// 隐蔽版：为了"声明在一起"而提前默认构造
void process() {
    Price p;                    // 默认构造（万一 Price 没有廉价默认构造呢？）
    Qty q;
    if (!decode_header(buf, p)) return;    // p 构造了却可能没用上
    if (!decode_body(buf, q)) return;      // q 同理
    match(p, q);
}

// 正解：解码函数按值返回，变量在拿到值的那一刻才出生
void process() {
    auto p = decode_header(buf);  if (!p) return;
    auto q = decode_body(buf);    if (!q) return;
    match(*p, *q);                // optional 或直接返回（→ item21 按值返回）
}
```

热路径解码尤其如此：**没有默认构造 + 可能早退 = 别提前定义**。
`std::optional<T>` / 工厂返回值让"变量出生就有值"成为默认形态。

## HFT 关联

- 热循环里的临时对象：默认选循环内定义（B）——现代编译器对 POD 会直接
  寄存器分配，成本为零；只有**大 buffer 复用**才值得循环外（且必须写明意图）
- 早退密集的解析路径：变量延后到"校验通过"之后——失败路径零构造零析构，
  失败率高的场景（防火墙式过滤）收益直接可见
- 与 `optional`/`expected` 搭配：C++17 后"先判错再定义"可以写成
  `auto tick = decode(buf); if (!tick) return;`——错误处理与延后定义合一

## 代码自测

**题目 1：** "尽可能延后"和"循环外只构造一次"矛盾吗？

<details>
<summary>参考答案</summary>

不矛盾——本条款管的是"**在使用前不要无谓构造**"（异常路径/早退/默认构造浪费），
循环外定义管的是"**有意复用**"（n 次赋值 < n 次构造析构时）。
判别：变量在循环每次迭代都需要**全新状态** → 循环内定义（顺便防残留 bug）；
你就是要**复用它的存储**（大 buffer/池对象）→ 循环外定义，并把复用意图写明白。
两者共同的敌人是"无意识的默认构造"——那才是本条款要消灭的。

</details>

**题目 2：** 延后定义对异常安全有什么具体收益？

<details>
<summary>参考答案</summary>

异常抛出路径上，**已构造的局部对象都要析构**——定义得越早，异常路径的
析构清单越长（展开成本越大，见 18.1 ①）。延后定义 = 异常路径上还没出生的对象
不用析构：① 省掉构造+析构的纯浪费；② 栈展开的帧更轻；
③ 更重要的是**异常安全推理更简单**——"异常时哪些对象已存在"一眼可见。

</details>

**题目 3：** 什么情况下"循环内定义"反而比"循环外定义"快？

<details>
<summary>参考答案</summary>

当 **构造+析构比赋值便宜** 或**赋值的旧值清理很贵**时：
典型例子是含堆内存的类型——循环外的 `w = make(i)` 每次赋值要释放旧分配
再新分配（或复用但 shrink 逻辑）；循环内的 `Widget w = make(i)` 是移动构造，
RVO 下常常连移动都省掉（→ item21 实测）。此外循环内定义让编译器确切知道
变量的生存域，**寄存器分配和死代码消除**都更激进——POD 类型循环内定义
经常编译成"根本没有这个变量"。

</details>
