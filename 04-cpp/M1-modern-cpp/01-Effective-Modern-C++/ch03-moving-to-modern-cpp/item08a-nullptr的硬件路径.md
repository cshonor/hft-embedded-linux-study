# Item 8a：nullptr 的硬件路径——判空、重载与 0 页保护（汇编实测）

> 第 3 章 移步现代 C++ · Item 8 配套深挖 · 上一节：[Item 8 优先 nullptr](item08-nullptr.md)

## 为什么要学这个（先建立直觉）

Item 8 在**语言层**回答了"为什么用 `nullptr`"（重载歧义、模板推导）。但学 C/C++ 的人迟早会问一个更底层的问题：

> `if (p)` 这行代码到了 CPU 上到底是什么？`nullptr`、`NULL`、`0` 三种写法在机器层面有区别吗？

答案先说：**没有区别——三种写法编出来的机器码逐字节相同**。C++ 的类型系统（`std::nullptr_t`、重载决议）是**编译期**的事；到了汇编层，空指针就是数值 0，判空就是判零，和 C 走完全相同的硬件路径。

本文所有汇编均为实测（clang 23.1.0，macOS arm64；x86-64 为同编译器交叉生成）。

---

## 这节讲什么

1. C++ 三种判空写法的汇编对照——证明 `std::nullptr_t` 是零开销抽象
2. C 与 C++ 的 `if (p)` 汇编一致性——语言不同，硬件路径相同
3. `NULL` 在本机的真实定义，以及 `g(NULL)` **直接编译失败**的实测（比 Item 8 说的"误选 int"更糟）
4. 重载决议实测：`g(0)` / `g(nullptr)` 各选了谁
5. 0 页保护对 C++ 一视同仁：解引用 `nullptr` 照样 SIGSEGV

---

## 核心机制

### 实测 1：三种写法，同一份机器码

测试源码：

```cpp
int f_if_p(int *p)       { if (p)            return 1; return 0; }
int f_eq_nullptr(int *p) { if (p == nullptr) return 1; return 0; }
int f_eq_NULL(int *p)    { if (p == NULL)    return 1; return 0; }
```

ARM64（clang -O1）实测输出：

```asm
__Z6f_if_pPi:                   // if (p)
    cmp  x0, #0
    cset w0, ne
    ret

__Z12f_eq_nullptrPi:            // if (p == nullptr)
    cmp  x0, #0
    cset w0, eq
    ret

__Z9f_eq_NULLPi:                // if (p == NULL)
    cmp  x0, #0
    cset w0, eq
    ret
```

三个函数体**完全相同**（只有 `if(p)` 的 `ne` 与另两个的 `eq` 是语义取反）。x86-64 同样：

```asm
_Z6f_if_pPi:                    # if (p)
    xorl  %eax, %eax
    testq %rdi, %rdi            # 判零：p AND p
    setne %al
    retq

_Z12f_eq_nullptrPi:             # if (p == nullptr)
    xorl  %eax, %eax
    testq %rdi, %rdi            # 一模一样
    sete  %al
    retq
```

**结论：`nullptr` 的类型安全性在运行期成本为零。** `std::nullptr_t` 的全部价值发生在编译期（重载决议、模板推导），编完就消失了。

### 实测 2：C 与 C++ 的 `if (p)` 汇编一致

同一份 `int f_if_p(int *p) { if (p) return 1; return 0; }`：

| 语言 | x86-64 -O1 | ARM64 -O1 |
|------|-----------|-----------|
| C | `testq %rdi,%rdi; setne %al` | `cmp x0,#0; cset w0,ne` |
| C++ | `testq %rdi,%rdi; setne %al` | `cmp x0,#0; cset w0,ne` |

逐字节相同（除符号名 mangling）。**CPU 不认识 C 也不认识 C++，只认识 64 位二进制数。**

### 实测 3：`g(NULL)` 在本机直接编译失败

Item 8 说 `g(NULL)` "可能误选 `g(int)`"。macOS arm64 + clang 23.1.0 实测更狠——**根本编译不过**：

```cpp
#include <cstdio>
void g(int)   { puts("g(int)"); }
void g(char*) { puts("g(char*)"); }

int main() {
    g(0);        // OK：g(int)
    g(NULL);     // ❌ error: call to 'g' is ambiguous
    g(nullptr);  // OK：g(char*)
}
```

编译器报错原文：

```
error: call to 'g' is ambiguous
note: candidate function: void g(int)
note: candidate function: void g(char*)
```

根因在本机 `NULL` 的定义：

```bash
$ echo | clang++ -E -dM -x c++ -include cstdio - | grep "define NULL"
#define NULL __DARWIN_NULL        # C++ 模式下展开为 clang 内建 __null
```

`__null` 是个"半指针半整数"的内建常量——到 `int` 和到 `char*` 的转换序列**评级相同**，重载决议无法裁决 → 歧义错误。也就是说在不同平台上 `NULL` 的命运有三种：**误选 int（Linux 传统 `0L`）、编译失败（macOS `__null`）、碰巧正确**——只有 `nullptr` 在所有平台上行为一致。

### 实测 4：重载决议

去掉 `NULL` 行后运行实测（本机 arm64）：

```
g(0)        → g(int)      ✅ 0 是 int 字面量，精确匹配整型重载
g(nullptr)  → g(char*)    ✅ nullptr 只能转指针，匹配指针重载
```

### 实测 5：解引用空指针，C++ 一样崩

```c
int main(void) { int *p = (int*)0; return *p; }
```

本机运行实测：`exit=139`（= 128 + SIGSEGV(11)）。macOS arm64 的 `__PAGEZERO` 段把低 **4GB** 地址全部保留不映射，`nullptr` 解引用第一步就踩进陷阱。**0 页保护对 C 和 C++ 一视同仁**——它是 OS 页表设计，不认语言。

### 硬件路径全景

```
if (p)  /  if (p == nullptr)  /  if (p == NULL)     ← C++ 源码（三选一）
  ↓ 编译期：std::nullptr_t 类型检查、重载决议（C++ 特有，零运行开销）
  ↓
testq %rdi, %rdi  /  cmp x0, #0                     ← 汇编：判零
  ↓
ZF/Z 标志 → setne/sete / 条件跳转                    ← CPU 执行
─────────────────────────────────
*p（解引用）→ 地址 0 → __PAGEZERO / mmap_min_addr 无映射 → SIGSEGV
```

---

## 常见错误（新手踩坑）

**错误 1：以为 `nullptr` 判空比 `NULL` 慢**
```cpp
if (p == nullptr) { ... }   // 有人担心"类型安全有代价"
```
**事实：** 实测 1 证明三种写法机器码逐字节相同。类型安全是编译期属性，运行期零开销。

**错误 2：跨平台代码用 `NULL` 传空指针给重载函数**
```cpp
register_cb(NULL);   // Linux：可能静默选错；macOS：直接编译失败
```
**修正：** 一律 `register_cb(nullptr);`——全平台行为一致。

**错误 3：以为 `if (p)` 会访问 p 指向的内存**
**事实：** 判空只读 `p` 这个变量本身（在寄存器里），`test`/`cmp` 不访存。只有 `*p` 才访问地址 0。

**错误 4：在 C++ 里用 `(void*)0` 显式模仿 C 的 NULL**
```cpp
int *p = (void*)0;    // ❌ C++ 不允许 void* 隐式转 int*，显式 cast 虽能编过但是恶习
```
**修正：** `int *p = nullptr;`。

---

## 新手要点（和 C 的区别）

| 维度 | C | C++ | 汇编层 |
|------|---|-----|--------|
| 空指针写法 | `NULL`（`((void*)0)`） | `nullptr`（`std::nullptr_t`） | **完全相同** |
| 重载歧义 | 无重载，无此问题 | `0`/`NULL` 危险，`nullptr` 安全 | 编完就消失 |
| 模板推导 | 无模板 | `nullptr` → `nullptr_t` 正确 | 编完就消失 |
| 判空机器码 | `test`/`cmp` | `test`/`cmp` | **逐字节相同** |
| 解引用崩溃 | SIGSEGV（0 页保护） | SIGSEGV（同一机制） | **完全相同** |

**一句话总结：** C++ 的 `nullptr` 是**编译期的类型安全补丁**；运行时它和 C 的 `NULL` 是同一个数值 0，走同一条硬件路径，掉同一个 0 页陷阱。

---

## HFT 关联

- **热路径判空零成本**：`if (!order_ptr) return;` 编译后就是一条 `test`+分支，与 C 完全相同——C++ 抽象不带运行期税。
- **回调注册的类型安全**：`set_handler(nullptr)` 清除回调在所有平台行为一致；`set_handler(NULL)` 在 macOS 上编译失败、在 Linux 上可能静默选错——**跨平台 HFT 基础设施代码必须 `nullptr`**。
- **模板工厂**：`template<class T> T* create() { return nullptr; }` 推导正确，且 `nullptr` 编出的返回代码就是 `xorl %eax,%eax; retq`（x86）/ `mov x0, xzr; ret`（ARM64）——零开销。
- **与 C 库互操作**：DPDK（C 库）返回的 `rte_mbuf*` 在 C++ 侧判空用 `if (mbuf == nullptr)` 与 C 侧 `if (mbuf == NULL)` 生成相同代码，混写无性能顾虑。

---

## 自测题

1. `if (p)`、`if (p == nullptr)`、`if (p == NULL)` 三种写法的汇编有区别吗？`std::nullptr_t` 的开销发生在什么时候？
2. 本机（macOS + clang）`g(NULL)` 在 `void g(int); void g(char*);` 重载集里的结果是什么？为什么比"误选 `g(int)`"更能说明问题？
3. C 的 `if (p)` 和 C++ 的 `if (p)` 汇编一致吗？这说明 CPU 层面"指针"是什么？
4. `__DARWIN_NULL` 展开为什么？它导致重载歧义的机制是什么？
5. 解引用 `nullptr` 的退出码 139 是怎么算出来的？macOS arm64 的低地址保护范围比 Linux 大多少倍？

<details>
<summary>参考答案</summary>

1. **没有区别，实测逐字节相同**（`cmp x0, #0` / `testq %rdi, %rdi`）。`std::nullptr_t` 的开销发生在**编译期**：类型检查、重载决议、模板推导都在编译时完成，编完类型信息就消失了，运行时它就是一个 64 位全 0 的数值。这是 C++"零开销抽象"原则的典型样本。

2. **直接编译失败**（`error: call to 'g' is ambiguous`）。这比"误选 `g(int)`"更能说明问题：`NULL` 的行为**依赖平台**——Linux 传统定义（`0L`）下静默选错（最危险，不报错），macOS 定义（`__null`）下编译失败（走运，能发现）。同一份代码换个平台从"编不过"变成"悄悄错"，只有 `nullptr` 全平台一致。

3. **完全一致**（除符号名 mangling 外逐字节相同：`testq %rdi, %rdi; setne %al`）。说明 CPU 层面**没有"指针"概念，只有二进制数**——指针判空与整数判零是同一条硬件路径，语言层的指针类型系统在汇编层不复存在。

4. `__DARWIN_NULL` 在 C++ 模式下展开为 clang 内建常量 **`__null`**。它是"半整数半指针"的魔法值：到 `int` 的转换与到 `char*` 的转换**重载评级相同**，编译器无法裁决 → ambiguity error。设计初衷是让 `NULL` 误用时更容易报错（相比 `0L`），但代价是语义仍然不精确。

5. 139 = 128 + 11，其中 11 是 **SIGSEGV** 的信号编号（shell 对"被信号杀死"的进程约定退出码 = 128 + 信号号）。macOS arm64 用 `__PAGEZERO` 保留低 **4GB**，Linux 默认 `mmap_min_addr` 保留低 **64KB**——前者是后者的 **65536 倍**（4GB / 64KB = 2³² / 2¹⁶ = 2¹⁶）。更大的保留区还能抓住"空指针 + 大偏移"（如 `p->field`，field 偏移数 MB）这类野访问。

</details>

---

## 参考与延伸

- 上一节：[Item 8 优先 nullptr 而非 0 和 NULL](item08-nullptr.md)——语言层的重载/模板论证
- 配套（C 侧，含 x86/ARM64 全套实测汇编与 0 页保护细节）：Pointers on C 6.14《空指针判空的硬件路径》
- 下一节：[Item 9 using 别名](item09-using.md)
- 回到：[第 3 章 移步现代 C++](README.md)
