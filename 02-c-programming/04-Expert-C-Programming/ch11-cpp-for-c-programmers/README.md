# 第 11 章 你懂得 C，所以 C++ 不在话下

**You Know C, C++ Is Easy!** / **C++ for C Programmers** — Peter van der Linden, *Expert C Programming*

## 本章目标

在 **[ch01–ch10](../README.md)** 建立的 **C 底层能力**（声明、链接、内存、数组、指针、**函数指针表**）之上，搭建 **C++ 桥梁**：理解 **OOP 三要素**（抽象/封装/继承多态）、**class 与 ctor/dtor/RAII**、**`virtual` 与 vtable/vptr**、**重载与运算符重载**、**iostream**；掌握 **C/C++ 混编严格性**（**`extern "C"`**、**name mangling**、**`void*`**、**`const` 链接**）；对照 **Linux `file_operations`** 与 **C++ 多态**；会用 **[c-to-cpp-migration-checklist.md](./c-to-cpp-migration-checklist.md)** 做 **安全迁移**。**ch11 是升华入口，不是放弃 C 地基**。

## C vs C++ 严格性速查

| 陷阱 | **C** | **C++** |
|------|-------|---------|
| **`void* → T*`** | 隐式 ✅ | **显式 cast** |
| **全局 `const int x=1` 链接** | 常 **外部** | 默认 **内部** — 需 **`extern const`** |
| **符号链接** | **`foo`** | **mangled** — C 接口 **`extern "C"`** |
| **内存** | **`malloc/free`** | **`new/delete`** — **禁止混用** |
| **布尔** | **`int` 0/1** | **`bool` / `true` / `false`** |
| **IO** | **`printf`** 格式串 | **`cout`** 类型驱动 |
| **多态** | **手动 ops 表** | **vptr + vtable** |

见 **11.17**、**demo01_extern_c**、**demo04_new_delete**。

## 多态：C ops 表 vs C++ virtual

| | **C `file_operations`** | **C++ `virtual`** |
|--|-------------------------|-------------------|
| **表** | **`f_op` 指向 ops** | **vptr → vtable** |
| **绑定** | **`f->f_op->read(...)`** | **`p->read()`** |
| **谁维护** | 驱动作者 | 编译器 |
| **典型场景** | **内核/DPDK** | **应用/框架** |
| **对象头** | 可选 **ops*** | 通常 **vptr 在对象首** |

见 **11.13–11.16**、**11.15**、**demo03_c_vs_cpp_polymorph**。

## vptr 内存布局（常见 ABI）

```text
无 virtual:  [ 成员数据... ]
有 virtual:  [ vptr ][ 基类数据 ][ 派生数据... ]
              ↓
            vtable (.rodata, 每类一份)
```

见 **11.14 解释**、**11.16 新奇玩意**。

## STL vs C 库对照（节选）

| 需求 | **C** | **C++ STL** |
|------|-------|-------------|
| 动态数组 | **`malloc/realloc`** | **`std::vector`** |
| 字符串 | **`char[]` + `str*`** | **`std::string`** |
| 关联容器 | 手写 + **`qsort`** | **`map` / `unordered_map`** |
| 排序/查找 | **`qsort` / `bsearch`** | **`sort` / `find`** |
| 内存 | **`malloc/free`** | **RAII / 智能指针** |

完整表见 **11.17**。

## 前置依赖

| 依赖 | 说明 |
|------|------|
| **[Expert C ch03](../ch03-analyzing-c-declarations/)** | 声明分析 — 读 C++ 声明延伸 |
| **[Expert C ch04–ch05](../ch04-arrays-are-not-pointers/)** | 数组≠指针、**链接/符号** |
| **[Expert C ch07](../ch07-adventures-in-memory/)** | 栈/堆、**`malloc`** |
| **[Expert C ch10](../ch10-more-about-pointers/)** | **函数指针**、**`void*`**、动态数组 — **C 多态祖先** |
| **[function-pointer-typedef-templates.md](../ch10-more-about-pointers/function-pointer-typedef-templates.md)** | ops 表 typedef 模板 |

## 知识模块

| 模块 | 小节 | 核心 |
|------|------|------|
| **1 OOP 入门** | **11.1–11.2** | OOP 三要素；C 兼容与严格性预告 |
| **2 封装与类** | **11.3–11.5** | **`class`/`struct`**；**`public/private/protected`** |
| **3 声明与调用** | **11.6–11.7** | 引用、**`inline`**；**`this`** |
| **4 继承** | **11.8–11.9** | 单继承；**多继承/菱形** |
| **5 重载** | **11.10–11.11** | 函数重载；**运算符重载** |
| **6 IO** | **11.12** | **`cout/cin`** vs **`printf/scanf`** |
| **7 多态** | **11.13–11.16** | **`virtual`**；**vtable/vptr**；**`file_operations`** |
| **8 C++ 要点** | **11.17** | **`const/constexpr`**、引用、**`new/delete`**、namespace、模板、**`extern "C"`** |
| **9 哲学** | **11.18–11.19** | C vs C++ **目标与权衡** |
| **10 收束** | **11.20–11.21** | 幽默；延伸阅读；**全书回顾** |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01_extern_c** | **`extern "C"`**；name mangling；C 调 C++ | **11.10, 11.17** |
| **demo02_ref_vs_ptr** | **`int&`** vs **`int**`** 出参 | **11.6–11.7, 11.17** |
| **demo03_c_vs_cpp_polymorph** | C **ops 表** vs **`virtual`**；vptr 地址 | **11.13–11.16** |
| **demo04_new_delete** | **`new/delete`** vs **`malloc/free`**；配对 | **11.4, 11.17** |

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Expert-C-Programming/ch11-cpp-for-c-programmers/demo
cd demo01_extern_c && make && ./demo01 && cd ..
cd demo02_ref_vs_ptr && make && ./demo02 && cd ..
cd demo03_c_vs_cpp_polymorph && make && ./demo03 && cd ..
cd demo04_new_delete && make && ./demo04 && cd ..
```

## 高频考点 / 面试题

1. **C++ 如何调用 C 函数？C 如何调用 C++ 函数？什么是 name mangling？** → C++ 调 C：**`extern "C"` 声明**；C 调 C++：C++ 侧 **`extern "C"` 导出**；mangling 编码 **重载/namespace** 到符号名（**11.10, 11.17**, **demo01_extern_c**）

2. **C 的 `file_operations` 与 C++ `virtual` 多态有何异同？** → 同：**运行时查表**；异：C **手动绑表**、可 **无 per-object ops***；C++ **vptr 在对象内**、编译器生成 vtable（**11.14–11.15**, **demo03**）

3. **含 `virtual` 的对象内存布局？`sizeof` 为何变大？** → 对象首常 **vptr**；**+指针宽** + 对齐 padding；vtable **每类一份** 在 rodata（**11.14, 11.16**）

4. **`malloc` 与 `new` 能否混用？引用与指针区别？** → **禁止** `free`/`new` 混用；**引用** 必绑定对象、无空、语法像别名（**11.17**, **demo02**, **demo04**）

5. **Linux 内核为何主要用 C 而非 C++？** → **确定性/可预测布局**、**无异常/RTTI/全局 ctor**、**历史与 ABI**、**ops 表足够**；用户态工具可用 C++（**11.15, 11.18–11.19**, **checklist §7**）

**拓展：**

- **`typedef char* Str; const Str s` 在 C++ 中？** → **`char* const`**（**ch10 10.4**）
- **构造函数里调 `virtual` 为何不派生？** → **vptr 随构造阶段更新**（**11.14**）
- **模板与 `void*` 选型？** → 编译期类型安全 vs 单份二进制（**11.17**）

## 全书收束：ch1–10 → ch11

```text
ch01–02  历史、预处理        → 懂宏的代价 → constexpr
ch03     声明螺旋            → 读 C++ 成员/引用声明
ch04–05  数组≠指针、链接      → extern "C"、ODR
ch06–07  运行时、内存         → RAII、new/delete
ch08     规则之难            → C++ 规则层更多
ch09–10  数组、指针、函数表   → vtable 的 C 祖先
ch11     C++ 桥梁            → 带着底层读 OOP，而非放弃底层
```

## 辅助材料

- **[c-to-cpp-migration-checklist.md](./c-to-cpp-migration-checklist.md)** — C → 安全 C++ **一页对照改写清单**
- **[ch10 function-pointer-typedef-templates.md](../ch10-more-about-pointers/function-pointer-typedef-templates.md)** — C 侧 ops/回调模板

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **[ch10](../ch10-more-about-pointers/)** 指针与函数表；**[ch07](../ch07-adventures-in-memory/)** 内存 |
| 关联 | **[ch05](../ch05-thinking-of-linking/)** 链接；**[ch08](../ch08-halloween-vs-christmas/)** 软件规则 |
| 延伸 | 仓库 **`01-C++Primer/`**；*Effective Modern C++* |
| 全书 | **ch01–10 C 地基 → ch11 C++ 桥梁 → 专项 C++/内核路线** |

## 小节

- [11.1 初识 OOP](./11.1-初识OOP.md)
- [11.2 抽象——取事物的本质特性](./11.2-抽象取事物的本质特性.md)
- [11.3 封装——把相关的类型、数据和函数组合在一起](./11.3-封装把相关的类型-数据和函数组合在一起.md)
- [11.4 展示一些类——用户定义类型享有和预定义类型一样的权限](./11.4-展示一些类用户定义类型享有和预定义类型一样的权限.md)
- [11.5 访问控制](./11.5-访问控制.md)
- [11.6 声明](./11.6-声明.md)
- [11.7 如何调用成员函数](./11.7-如何调用成员函数.md)
- [11.8 继承——复用已经定义的操作](./11.8-继承复用已经定义的操作.md)
- [11.9 多重继承——从两个或更多的基类派生](./11.9-多重继承从两个或更多的基类派生.md)
- [11.10 重载——作用于不同类型的同一操作具有相同的名字](./11.10-重载作用于不同类型的同一操作具有相同的名字.md)
- [11.11 C++ 如何进行操作符重载](./11.11-C如何进行操作符重载.md)
- [11.12 C++ 的输入/输出 (I/O)](./11.12-C的输入输出-IO.md)
- [11.13 多态——运行时绑定](./11.13-多态运行时绑定.md)
- [11.14 解释](./11.14-解释.md)
- [11.15 C++ 如何表现多态](./11.15-C如何表现多态.md)
- [11.16 新奇玩意——多态](./11.16-新奇玩意多态.md)
- [11.17 C++ 的其他要点](./11.17-C的其他要点.md)
- [11.18 如果我的目标是那里，我不会从这里起步](./11.18-如果我的目标是那里-我不会从这里起步.md)
- [11.19 它或许过于复杂，但却是唯一可行的方案](./11.19-它或许过于复杂-但却是唯一可行的方案.md)
- [11.20 轻松一下——死亡计算机协会](./11.20-轻松一下死亡计算机协会.md)
- [11.21 更多阅读材料](./11.21-更多阅读材料.md)
