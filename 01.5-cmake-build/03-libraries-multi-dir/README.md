# 第 3 章 · 库与多目录：工程长大之后

> **本章讲什么：** [第 1 章](../01-minimal-c-project/README.md)一个 `add_executable` 装下全部源码，那是 3 个文件的待遇。
> 工程长到几十个文件，就要拆：拆成**库**（可复用的代码块）和**目录**（一个模块一个家）。
> 这章用 `demo/` 的三层小工程（app → risk → ma）把现代 CMake 的组织术一次讲完：
> `add_subdirectory` / `add_library` / `target_link_libraries`，
> 以及 PRIVATE / PUBLIC / INTERFACE 到底怎么选——用一个真实的编译错误来记。
>
> 前置：第 1 章。说明：命令与输出按 CMake 3.16+ 标准格式整理，本机可直接复跑。
> **示例工程：** [`demo/`](./demo/)——三层小工程（app → risk → ma），可亲手复跑。

## 章节导航

| 节 | 标题 | 一句话 |
|----|------|--------|
| [3.1](./3.1-一行写下40个文件名的那天.md) | 一行写下 40 个文件名的那天 | 单 target 的三笔账：全量重链 / 零复用 / 边界模糊 |
| [3.2](./3.2-库是打包好的一组o.md) | 库就是"打包好的一组 .o" | STATIC vs SHARED 一张表；HFT/嵌入式默认 STATIC |
| [3.3](./3.3-一棵树每个节点一个target.md) | 一棵树，每个节点一个 target | `add_subdirectory` 目录自治 + `target_link_libraries` 依赖声明 |
| [3.4](./3.4-PRIVATE-PUBLIC-INTERFACE.md) | PRIVATE / PUBLIC / INTERFACE | 用一次真实的 `file not found` 记住关键字怎么选 |
| [3.5](./3.5-链接顺序与undefined-reference怎么读.md) | 链接顺序与 undefined reference | 三步读法；CMake 按依赖图自动排序，`link.txt` 可验证 |
| [3.6](./3.6-本章验收清单.md) | 本章验收清单 | 五条自查（含亲手复现报错） |
| [3.7](./3.7-与后续衔接.md) | 与后续衔接 | 第 4 章 Zephyr 多目录放大 / 第 5 章编译选项 / 加模块小练习 |

> 快速上手（30 秒版）：`cmake -B build && cmake --build build && ./build/app/feed_handler`

## 代码自测

<details><summary>Q1：add_subdirectory 和 target_link_libraries 各解决什么问题？</summary>

`add_subdirectory` 解决**代码组织**：让每个子目录自治——自己的 CMakeLists 创建自己的
target，顶层不需要知道细节（解耦 + 可扩展）。`target_link_libraries` 解决**依赖声明**：
告诉 CMake 谁依赖谁，CMake 据此自动完成编译排序、链接命令拼装、PUBLIC 属性传播。
一句话：前者管"工程长什么样"，后者管"target 之间怎么连"。
</details>

<details><summary>Q2：risk 库内部用了 ma，为什么 target_link_libraries(risk PRIVATE ma) 而不是 PUBLIC？</summary>

因为 ma 只是 risk 的**实现细节**：risk.c 调 ma_compute，但 risk.h 里没有暴露任何 ma 的
类型或函数。用 risk 的 target（app）不需要知道 ma 存在。PRIVATE 的判定标准：
"我的**头文件**里有没有用到这个依赖？"——有则 PUBLIC，没有则 PRIVATE。
这个写错不是语法问题而是封装问题：PRIVATE 写成 PUBLIC 会把内部依赖泄给所有使用者，
将来想换掉 ma 时牵动全身。
</details>

<details><summary>Q3：target_include_directories 的 PUBLIC/PRIVATE 和 target_link_libraries 的是同一套语义吗？</summary>

同一套传播语义，作用对象不同。两者都回答"这个属性传不传给链接我的 target"：
include 路径 PUBLIC = 我的头文件在对外接口里，用我的人必须能 include 到；
链接库 PUBLIC = 我的头文件里用了依赖的类型，用我的人编译时也需要那个依赖的头。
判定方法完全一致：**看头文件（对外接口），不看 .c（实现）**。
INTERFACE 则是"我自己编译不用、只给用我的人"——header-only 库的标配。
</details>

<details><summary>Q4：链接报 undefined reference to 'ma_compute'，可能的原因有哪些？</summary>

按概率排：①根本没链 ma——检查 target_link_libraries 声明是否漏了；
②静态库顺序错——手写链接命令时 librisk.a 必须在 libma.a 之前
（CMake 声明式依赖自动排对，手写 Makefile 才容易踩）；
③符号真的不存在——函数名拼错、声明与定义签名不一致（C++ 还会因 name mangling 放大此坑）；
④库编出来了但 .a 是旧的——删掉 build/ 重配。排查工具：link.txt 看实际链接命令，
nm libma.a 看符号在不在。
</details>

<details><summary>Q5：为什么说静态库是"按需取用"？整个 libma.a 都会进最终二进制吗？</summary>

不会。静态库是 .o 的归档包，链接器处理它时只把**能补齐当前未定义符号**的 .o 拆出来。
libma.a 里有 ma.c.o 和从未被调用的 stats.c.o，最终二进制只含 ma.c.o。
推论：静态库可以做得很大而不用担心体积——没用到的模块不会进产物；
但反过来，"库里编进去了"不等于"程序里有"，排查符号问题时要用 nm 看的是
最终二进制而不是 .a。
</details>
