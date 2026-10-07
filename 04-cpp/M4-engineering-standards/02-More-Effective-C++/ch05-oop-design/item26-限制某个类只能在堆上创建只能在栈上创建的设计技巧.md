# 条款 26：限制某个类只能在堆上/只能在栈上创建的设计技巧

## 本节讲什么

**Restrict objects to heap-only or stack-only creation.** 有些类型的正确性依赖
创建位置：池化对象必须来自池（不能在栈上逃逸）、RAII 守卫必须在栈上
（堆分配它就失去意义）。本条给出两种约束的实现，以及一个实测踩出的坑：
**私有析构会把 unique_ptr 也挡在门外**（需要友元 deleter）。
本机 g++ 13.3 实测两种约束与修复。

← 上一条 [item25 虚继承开销](./item25-虚拟继承（virtualpublic）的底层实现、巨大开销，能不用就不用.md)；
下一条 [item27 RTTI 开销](./item27-剖析运行时类型识别RTTI（dynamic_casttypeid）的开销与合理使用场景.md)。

---

## 1. 只能堆上创建：私有析构 + 工厂（含实测踩坑）

```cpp
class HeapOnly {
    ~HeapOnly() = default;                          // 私有析构：栈对象编译不过
    friend struct std::default_delete<HeapOnly>;    // ⚠ 关键：unique_ptr 的 deleter 放行！
public:
    static std::unique_ptr<HeapOnly> create() {
        return std::unique_ptr<HeapOnly>(new HeapOnly);
    }
};
auto h = HeapOnly::create();        // ✅
// HeapOnly stack_h;                // ❌ 编译错误（析构私有）
```

**实测踩出的坑**（g++ 13.3 如实记录）：第一版没写 friend 声明，
`unique_ptr<HeapOnly>` **编译失败**——`default_delete` 的 `operator()`
也要调析构，同样被私有挡住。三个解法：
1. **友元 deleter**（上例，本机实测通过）——`friend struct std::default_delete<HeapOnly>;`
2. **自销毁接口**：`void destroy() { delete this; }`——返回裸指针配显式销毁
   （C 风格 API 常用，但忘记 destroy 就泄漏）
3. **自定义 deleter**：`unique_ptr<HeapOnly, MyDeleter>`（deleter 是类的友元）

**用途**：对象寿命必须被明确管理（注册到全局表的监听器——栈对象销毁后
表里悬着野指针）、禁止 accidental copy（栈对象按值传递）。

## 2. 只能栈上创建：删除 operator new（实测）

```cpp
class StackOnly {
    static void* operator new(std::size_t) = delete;      // 删普通版
    static void* operator new[](std::size_t) = delete;    // 删数组版（别漏！）
public:
    StackOnly() = default;
};
StackOnly s;                    // ✅
// auto* p = new StackOnly;     // ❌ 编译错误（本机实测）
```

**三个注意**：
- `operator new[]` 要单独删（数组版独立于普通版，→ Effective item51 常规）
- **scoped/静态/成员**存储都不受影响——删除的只是 `new` 表达式
- 想"只能来自池"的变体：不删 new，而是**重载** operator new 走池
  （→ item49 类内重载：约束 + 池化一石二鸟）

**用途**：RAII 守卫（lock_guard 这类——堆分配它语义就错了）、
热路径小对象（强制内联存储在容器/栈上，不许散落到堆）。

## 3. 边界与漏洞（诚实清单）

| 约束 | 挡不住的 |
|---|---|
| HeapOnly（私有析构） | 放在**别的对象里**当成员（宿主在栈上它就间接在栈上）——要挡需要连"作为成员"也禁（删除构造的可达性分析，工程上靠约定） |
| StackOnly（删 new） | `std::make_shared<StackOnly>()`——make_shared 用自己的分配函数，绕开了类内 operator new！（它也 `new`，但走的是**全局** new——类内 delete 的是类内重载，全局 new 管不到） |

第二行是真实漏洞：类内 `operator new = delete` 只影响 `new StackOnly` 表达式——
`make_shared`/`make_unique` 内部用**全局** `::operator new`（除非类内重载被找到——
已删除的定义会让它们也失败，实测行为：调用到 deleted 定义照样编译错误，
但某些分配器路径可能完全绕过类作用域）。**结论：约束是"默认值引导"，
不是铜墙铁壁**——真正的纪律靠命名（`xxxGuard` 一看就该在栈上）+ review。

## HFT 关联

- **池化订单对象用 StackOnly 变体**（重载 new 走池）：
  `new Order` 进池、栈上 Order 禁止（池化语义强制——
  对象寿命归池管，栈上的没人回收，→ item18 池化 + item49 类内重载的合体）
- RAII 的时序守卫（`ScopedTimer`/`RiskGuard`）标 StackOnly：
  堆分配一个"作用域计时器"本身就是 bug（它该随作用域生灭）
- HeapOnly 的友元 deleter 坑在**框架代码**最常踩：
  "工厂返回 unique_ptr 却编译不过"——三个解法里友元版最干净（本机实测）

## 代码自测

**题目 1：** 私有析构为什么能挡住栈对象？unique_ptr 为什么也会中招？

<details>
<summary>参考答案</summary>

栈对象的析构在**作用域结束点**由编译器生成调用——
析构私有 = 这处调用编译错误，栈对象无法合法存在。
`unique_ptr<HeapOnly>` 中招是同一原理：`default_delete<HeapOnly>::operator()`
里写着 `delete ptr;`——也要访问私有析构（本机实测编译失败：
"~HeapOnly() is private within this context"）。
解法：`friend struct std::default_delete<HeapOnly>;`（实测通过）、
或自销毁接口 `destroy() { delete this; }`、或自定义 deleter 配友元。
这是"HeapOnly 配智能指针"的经典陷阱——
设计约束时要把**所有析构调用点**想全（栈、智能指针、容器、delete 表达式）。

</details>

**题目 2：** `operator new = delete` 能彻底阻止堆创建吗？说出两个绕过路径。

<details>
<summary>参考答案</summary>

**不能彻底**——它只删除 `new StackOnly` 表达式的类内重载：
① **全局 new 路径**：显式 `::new StackOnly`（全局作用域的 operator new
不受类内删除影响——除非类内重载本来就存在并被找到，但 deleted 定义
被找到时照样编译错误，所以这条在严格语义下也堵得住；真正的漏洞在②）；
② **工厂函数的分配路径**：`std::make_shared<StackOnly>()` 的分配
由 make_shared 自己控制（它用全局分配器或控制块一体分配）——
类内 delete 够不着它（行为随实现/调用形态而异，不可靠）。
结论：删 operator new 是"默认值引导"（让常规写法编译失败），
不是安全边界——`StackOnlyGuard` 的命名 + review 才是最后的墙。
要"只能来自池"，正解是**重载** operator new 走池（item49），
让堆创建本身就被导向正确的地方。

</details>

**题目 3：** 池化订单对象想强制"只能来自池"，为什么"重载 new 走池 + 禁栈创建"比"删 new"更对？

<details>
<summary>参考答案</summary>

三个理由：
① **语义对齐**：池化对象的正确性不是"不许在堆上"，
而是"**寿命必须归池管**"（谁 acquire 谁 release，→ item18）——
重载 new 走池让"堆创建"自动等于"池创建"，约束与机制合一；
删 new 只是禁了一条路，池语义并没有被强制。
② **可用性**：`new Order` 的写法完全保留（调用方无感），
底层从 malloc 变池——删 new 则要求所有调用方改用工厂函数（侵入大）。
③ **配套完整**：类内重载天然带上 operator delete（归还池，
→ item49）——创建和归还的配对在语言层闭合；
删 new 管不了"delete 池对象走 free"的错配。
完整形态：类内 operator new（走池）+ 私有析构或文档约束
（栈对象挡不住，靠 `Order` 不经手栈分配的纪律 + review）——
约束能机制化的部分机制化，剩下的诚实承认靠纪律。

</details>
