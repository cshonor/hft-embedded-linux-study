# 条款 27：剖析 RTTI 的开销与合理使用场景

## 本节讲什么

**Understand RTTI's costs and legitimate uses.** RTTI 的机制与实测已在 **18.4**
完整展开（指针/引用版失败语义、typeid 动静区分、-fno-rtti 边界），
本条做**成本量化与使用准则**的收口：RTTI 在哪些场景值回票价，
哪些场景是慢性毒药。

← 上一条 [item26 堆/栈约束](./item26-限制某个类只能在堆上创建只能在栈上创建的设计技巧.md)；
下一章 [ch06 智能指针](../ch06-smart-pointers/)。

---

## 1. RTTI 的成本分解（18.4 ③ 的量化版）

| 操作 | 成本 | 明细 |
|---|---|---|
| `typeid(*多态对象)` | ~O(1)，数 ns | vptr → type_info 指针解引用（一次间接） |
| `typeid(静态类型/表达式)` | **0**（编译期） | 常量折叠 |
| `dynamic_cast`（单继承浅链） | 几十 ns | 沿继承链比较 type_info（每级一次比较） |
| `dynamic_cast`（深链/MI/虚继承） | 上百 ns ~ µs | 链遍历 + 虚基表查询（→ 18.3 ②） |
| 类型名 `typeid(T).name()` | 免费但**不可移植** | mangled 名，仅调试用 |
| 二进制体积 | 每多态类一份 type_info | `-fno-rtti` 可省（18.4 ⑤） |

## 2. 合法使用场景（RTTI 值回票价的三个地方）

**场景一：插件/脚本边界的类型安全**

```cpp
// 插件加载后校验接口版本——一次性操作，成本无关
if (auto* v2 = dynamic_cast<IPluginV2*>(plugin)) {
    register_v2(v2);
} else if (auto* v1 = dynamic_cast<IPluginV1*>(plugin)) {
    register_legacy(v1);
}
```

启动期/加载期的转型，一次 RTTI 查询换 ABI 安全——冷路径的标准用法。

**场景二：调试与工具（typeid 的主场）**

日志/断言/测试里的类型打印（`typeid(*p).name()`）、
调试器的类型显示——生产热路径禁用，开发工具随便用。

**场景三：异构容器的一次性分拣**

入口处 `dynamic_cast` 一次后**缓存类型化指针**到分桶容器
（→ item23 的"合法残余"：转型成本摊一次，之后全程静态类型）。

## 3. 慢性毒药场景（RTTI 不该出现的地方）

- **每消息一次的分发热循环**（→ item23：msg_type 查表/虚函数分发替代）
- **替代虚函数的 if-else 链**（→ 18.4 ④：用 RTTI 手搓多态是最差形态）
- **热路径的类型断言**（"确定是它但转一下保险"——保险的成本每次都在付，
  类型信心该由设计保证，不该由运行期查询保证）

## HFT 关联

- 交易系统 RTTI 预算：**启动/加载路径随便用，热路径零出现**——
  `-fno-rtti` 编译热路径库是部分团队的选择（18.4 ⑤），
  折中是"热路径库不接 RTTI 依赖"（动态库边界分开编译）
- 插件型策略系统（第三方策略 .so 加载）的接口校验是 RTTI 的正当职业——
  加载一次 `dynamic_cast<IStrategyV3*>`，之后注册表分发全静态
- 对照 06.6.5 的方法论：怀疑 RTTI 成本时先 profile——
  `typeid` 几乎免费，`dynamic_cast` 深链才贵；"禁用 RTTI"和
  "禁用 dynamic_cast"是两个不同强度的决定，别混为一谈

## 代码自测

**题目 1：** `typeid` 和 `dynamic_cast` 的成本为什么差一个量级？

<details>
<summary>参考答案</summary>

`typeid(*p)`（多态对象）：vptr 里存着 type_info 指针，
解引用一次即得——**O(1)，几纳秒**（一次内存间接）。
`dynamic_cast<T*>(p)`：要回答"p 指向的对象**是不是** T 或 T 的派生"——
沿继承链逐级比较 type_info（每级一次字符串/指针比较），
MI/虚继承还要查虚基表算偏移（→ 18.3 ②）——**链越长越贵**，
深链/虚继承能到 µs 级。
所以"RTTI 慢"的准确说法是"**dynamic_cast 可能慢**"——
typeid 本身几乎免费，禁用 RTTI（-fno-rtti）省的是 type_info 体积
和 dynamic_cast 的路径，两者的决策强度不同。

</details>

**题目 2：** 插件系统加载第三方策略 .so，RTTI 的正确用法是什么？

<details>
<summary>参考答案</summary>

**加载时一次性校验，之后全静态**：

```cpp
IStrategy* load(const char* so) {
    void* h = dlopen(so, RTLD_NOW);
    auto* raw = dlsym(h, "create_strategy");
    IStrategy* s = ((IStrategy*(*)())raw)();
    if (auto* v3 = dynamic_cast<IStrategyV3*>(s)) { register_v3(v3); }
    else if (auto* v2 = dynamic_cast<IStrategyV2*>(s)) { register_v2(v2); }
    else { delete s; throw BadPlugin("接口版本不支持"); }
    return s;
}
```

要点：① `dynamic_cast` 在**加载边界**用一次（接口版本协商——
.so 是独立编译的，静态类型系统跨不过去，RTTI 是唯一的运行时类型凭证）；
② 校验后按版本**分桶注册**（v2 表/v3 表），之后引擎分发走各自
静态接口——RTTI 成本终生只付一次。
这就是"异构入口的一次性分拣"：转型摊销在冷路径，热路径零 RTTI。

</details>

**题目 3：** 为什么说"热路径的类型断言（确定是它但转一下保险）"是反模式？

<details>
<summary>参考答案</summary>

因为"保险"的成本**每次调用都在付**（dynamic_cast 的链遍历，
最坏 µs 级），而它防的错误（类型不符）应该由**设计保证**不可能发生：
① 如果类型确实保证——断言写在**调试构建**（`assert` + dynamic_cast，
发布构建归零），生产路径零成本；
② 如果类型不能保证——这是**设计缺陷**（类型知识泄漏到调用方，
→ item23），该修的是通道分桶/接口设计，不是每次运行期再查一次。
"运行期查询代替设计信心"的惯性会把动态类型的临时补丁变成永久成本——
热路径的类型信心必须来自类型系统（静态类型/variant/分桶容器），
不是来自每包一次的 RTTI 验证。

</details>
