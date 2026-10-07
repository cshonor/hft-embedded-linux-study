# 条款 34：C++ 和 C 混合编程的规范、链接、命名修饰与库兼容

## 本节讲什么

**Mix C and C++ correctly: linkage, name mangling, and library compatibility.**
C++ 靠**名字修饰**（name mangling）支持重载/命名空间——`feed_handler(int)`
在符号表里是 `_Z12feed_handleri`；C 没有修饰，符号就是 `gateway_recv`。
两种链接约定的交界处（C 库给 C++ 用、C++ 函数给 C 回调），
`extern "C"` 是唯一的桥。本机 g++ 13.3 + nm 实测修饰差异。

← 上一条 [item33 非叶子类抽象](./item33-把非叶子类设计为抽象类，强制约束子类实现接口，架构约束.md)；
下一条 [item35 标准演进](./item35-理解C++语言标准演进的方向，写与时俱进、不过时的代码.md)。

---

## 1. 名字修饰的实测对照（nm 证据）

```cpp
void feed_handler(int x) { /*...*/ }                    // 普通 C++ 函数
extern "C" void gateway_recv(const char* data, int len) { /*...*/ }   // C 链接
```

**本机实测**（`nm` 输出）：

```
0000000000000000 T _Z12feed_handleri      ← C++：_Z + 12(名长) + feed_handler + i(int 参数)
0000000000000050 T gateway_recv           ← extern "C"：纯名字，无修饰
```

**规则**：
- C++ 编译器默认修饰（重载需要签名进符号：`f(int)` 和 `f(double)` 是两个符号）
- `extern "C"` 关闭修饰——**该函数放弃重载/命名空间**（同名只能有一份）
- 后果：C 代码（或 C ABI 库）按纯名字 `gateway_recv` 找符号——
  没 extern "C" 的 C++ 函数它**根本找不到**（链接错误：undefined reference）

## 2. 三个典型场景与正确写法

**场景一：C 库给 C++ 用（头文件共享）**

```c
/* feed_api.h——C 库的头文件，要同时被 C 和 C++ 包含 */
#ifdef __cplusplus
extern "C" {
#endif

int feed_connect(const char* addr);
int feed_subscribe(int channel);
void feed_close(void);

#ifdef __cplusplus
}
#endif
```

`#ifdef __cplusplus` 守卫是标准形态：C 编译器看不到 extern "C"（C 不认识它），
C++ 编译器看到并关闭修饰——**同一个头，两种编译器各自正确**。
（交易所 SDK、系统库的头文件全是这个形态。）

**场景二：C++ 函数注册为 C 回调**

```cpp
extern "C" void on_tick_c(const TickC* t) {    // C 链接：C 库能按名字找到
    MyEngine::instance().on_tick(convert(t));  // 立即转入 C++ 世界
}
// 注册：c_library_set_callback(on_tick_c);——函数指针跨越语言边界
```

注意回调里的**异常纪律**：异常飞出 C 边界 = UB（→ MoreEff item13 的边界翻译层）。

**场景三：C++ 库给 C 用（反向输出）**

导出层（facade）：C++ 类包一层 extern "C" 的函数族 + 不透明指针——

```cpp
extern "C" {
    void* engine_create(const char* cfg) { return new Engine(cfg); }
    void  engine_run(void* e) { static_cast<Engine*>(e)->run(); }
    void  engine_destroy(void* e) { delete static_cast<Engine*>(e); }
}
// C 侧：void* 就是句柄——C 永远看不到 C++ 类型（opaque handle 模式）
```

## 3. 混编的其他三个坑

| 坑 | 内容 | 对策 |
|---|---|---|
| **头文件语言归属** | .h 被两种编译器包含时，C++ 语法（类/模板/引用）必须在 `#ifdef __cplusplus` 里 | 共享头只放 C 子集（POD/函数声明） |
| **静态初始化顺序** | C 库与 C++ 全局对象的初始化顺序跨语言不可控 | 显式 init 函数（`gateway_init()`）代替全局构造（→ Effective item47 的 NIF 手法） |
| **ABI 细节** | 结构体布局（对齐/位域）、bool 尺寸、枚举底层类型跨编译器可能不同 | 共享结构体定宽类型 + static_assert（→ 19.5 三件套） |

## HFT 关联

- **交易所 SDK 全是 C ABI**（FIX 引擎/柜台 API）——
  extern "C" 回调注册 + 异常边界翻译（item13）是每个网关的入门两件套
- 策略层 C++、网关层 C 的边界用 **opaque handle**（场景三）：
  C 侧永远拿不到 C++ 类型——ABI 边界稳定（C++ 类随便改，C 侧不用重编，
  → Effective item31 的编译防火墙同款思想）
- 共享行情结构体的跨语言布局：`#pragma pack`/定宽类型 + static_assert——
  C 的解析器和 C++ 的解码器对同一字节的理解必须钉死（→ 19.5 纪律）

## 代码自测

**题目 1：** 为什么 C++ 需要名字修饰而 C 不需要？extern "C" 放弃了什么？

<details>
<summary>参考答案</summary>

C++ 支持**重载**（同名不同参）和**命名空间**（同名不同域）——
纯函数名无法区分 `f(int)` 与 `f(double)`、`a::f` 与 `b::f`：
编译器把签名编进符号名（`_Z12feed_handleri` = _Z + 名长 + 名 + 参数类型码，
本机 nm 实测），链接器按修饰名精确匹配。
C 没有重载/命名空间——符号就是函数名本身，无需修饰。
`extern "C"` 放弃的就是这两样：该函数**不能重载**（同名只能一份）、
**不进命名空间**——换来 C ABI 兼容性（C 代码按纯名字找到它）。
所以 extern "C" 只用于"必须跨语言"的边界函数，C++ 内部函数滥用它
= 自废重载/命名空间武功。

</details>

**题目 2：** C 库的头文件要被 C++ 包含，标准写法是什么？各部分的原理？

<details>
<summary>参考答案</summary>

```c
#ifdef __cplusplus
extern "C" {
#endif
/* 函数声明（C 子集：无类/模板/引用） */
#ifdef __cplusplus
}
#endif
```

三段原理：① `__cplusplus` 宏只有 C++ 编译器定义——
C 编译器走不到 extern "C" 分支（C 不认识这个关键字，ifdef 把它藏起来）；
② extern "C" 块：C++ 编译器对块内声明关闭名字修饰——
与 C 库编译出的纯名符号对上（否则 C++ 按 `_Z...` 找符号，
undefined reference）；
③ 块内必须是 **C 子集**：函数声明用 C 类型（POD/指针/定宽整型），
类/模板/引用参数一律不许出现（C 编译器过不去）。
系统头（stdio.h/pthread.h）和交易所 SDK 全是这个形态——
照抄就是最稳的实践。

</details>

**题目 3：** C++ 的引擎要给 C 策略层用，设计导出层的三个要点？

<details>
<summary>参考答案</summary>

① **opaque handle**：C 侧只持有 `void*`（或 typedef 的句柄类型）——
C 永远看不到 C++ 类型定义，Engine 内部随便改（ABI 防火墙，
→ Effective item31 的编译依赖思想跨语言版）；
② **函数族导出**：create/run/destroy 全套 extern "C"——
生命周期函数（create/destroy）必须配对且文档写清所有权
（C 侧没有 RAII，泄漏防控靠纪律 + 句柄有效性校验）；
③ **异常边界**：每个导出函数内部 try-catch(...)——
异常飞出 extern "C" 边界 = UB（C 侧没有展开机制，
→ MoreEff item13 的位置二），错误必须翻译成返回值/错误码。
配套：版本协商函数（`engine_abi_version()`）——
C 策略和 C++ 引擎独立发版时，版本握手是生存线（→ item32 面向未来设计）。

</details>
