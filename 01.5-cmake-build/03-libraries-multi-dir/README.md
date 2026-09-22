# 第 3 章 · 库与多目录：工程长大之后

> **本节讲什么：** 第 1 章一个 `add_executable` 装下全部源码，那是 3 个文件的待遇。
> 工程长到几十个文件，就要拆：拆成**库**（可复用的代码块）和**目录**（一个模块一个家）。
> 这章用 `demo/` 的三层小工程（app → risk → ma）把现代 CMake 的组织术一次讲完：
> `add_subdirectory` / `add_library` / `target_link_libraries`，
> 以及 PRIVATE / PUBLIC / INTERFACE 到底怎么选——用一个真实的编译错误来记。
>
> 前置：第 1 章。说明：命令与输出按 CMake 3.16+ 标准格式整理，本机可直接复跑。

---

## 3.1 问题的起点：一行写下 40 个文件名的那天

`add_executable(app main.c a.c b.c ... nn.c)`——能跑，但三个账马上找上门：

| 账 | 表现 |
|---|---|
| 改一行全量重链 | 所有 .o 链进一个二进制，任何文件改动都触发整链，工程越大链接越慢 |
| 复用为零 | 另一个工程想要"移动平均"那部分代码？复制粘贴，两份代码从此各自漂移 |
| 边界模糊 | 谁都能 include 谁的 .c 私有头，模块间耦合成一团，没人说得清依赖关系 |

Makefile 时代的解法是自己写规则把 .o 打包成 `.a` 再链——`STM32-/labs/04` 那种
多模块工程就是手工维护这套。CMake 把这件事变成三个命令。

## 3.2 第一性原理：库就是"打包好的一组 .o"

`add_library(ma STATIC ma.c)` 产出的 `libma.a`，用 `ar` 的眼光看就是
`ma.c.o` 的压缩包。链接器在最终链接时**按需取用**：只把你用到的 .o 从包里拆出来链进去。

STATIC 和 SHARED 的区别，新手只需要一张表：

| | STATIC（.a / .lib） | SHARED（.so / .dylib / .dll） |
|---|---|---|
| 链接时机 | 编译期：代码**复制**进可执行文件 | 运行期：可执行文件只记"我要 libma.so"，启动时动态加载 |
| 产物 | 一个自包含的二进制 | 二进制 + 一堆 .so，部署要一起带 |
| 升级库 | 重新编译整个应用 | 换掉 .so 即可（ABI 兼容前提下） |
| 嵌入式/HFT 偏好 | **默认选它**：无运行时依赖、无虚函数式间接跳转、LTO 能跨库内联 | 插件系统、被多进程共享时才考虑 |

HFT 和嵌入式场景**默认 STATIC**：部署就是拷一个文件，没有"线上 .so 版本不对"这类事故。

## 3.3 demo/ 的骨架：一棵树，每个节点一个 target

```text
demo/
├── CMakeLists.txt          ← 顶层：只管 project() + add_subdirectory()
├── libs/
│   ├── ma/                 ← 移动平均库（底层，不依赖任何人）
│   │   ├── CMakeLists.txt  ← add_library(ma STATIC ma.c)
│   │   ├── ma.c / ma.h
│   └── risk/               ← 风控库：内部用 ma，但对外不暴露
│       ├── CMakeLists.txt  ← add_library(risk STATIC risk.c)
│       └── risk.c / risk.h
└── app/
    ├── CMakeLists.txt      ← add_executable(feed_handler main.c)
    └── main.c              ← 只 include "risk.h"
```

依赖图：`feed_handler → risk → ma`。两条规则看懂全部：

1. **`add_subdirectory(dir)`**：进入 dir 执行那里的 CMakeLists——
   每个目录**自治**：自己的源文件、自己的 target、自己的编译选项。
   顶层不知道 ma 库有几个 .c，也不应该知道。
2. **`target_link_libraries(A PRIVATE B)`**：声明 "A 依赖 B"——
   之后 CMake 自动做三件事：排好编译顺序（先 B 后 A）、
   链接时把 B 加进 A 的链接命令、把 B 标记为 PUBLIC 的属性传给 A。

构建输出（注意顺序：底层的 ma 先编）：

```text
[ 20%] Building C object libs/ma/CMakeFiles/ma.dir/ma.c.o
[ 40%] Linking C static library libma.a
[ 60%] Building C object libs/risk/CMakeFiles/risk.dir/risk.c.o
[ 80%] Linking C static library librisk.a
[ 90%] Building C object app/CMakeFiles/feed_handler.dir/main.c.o
[100%] Linking C executable feed_handler
```

## 3.4 PRIVATE / PUBLIC / INTERFACE：用一次真实报错来记

三者的区别只在一件事上：**这个属性要不要传给"链接我的 target"**。

| 关键字 | 我自己编译时用 | 依赖我的 target 也用 | 典型场景 |
|---|---|---|---|
| `PRIVATE` | ✓ | ✗ | 实现细节：.c 里 include 的头、内部用的库 |
| `PUBLIC` | ✓ | ✓ | 头文件里暴露的类型需要的路径/库 |
| `INTERFACE` | ✗ | ✓ | header-only 库；纯"接口"target |

demo/ 里的两个判定实例：

**判定一：`target_link_libraries(risk PRIVATE ma)` 为什么 PRIVATE？**
`risk.c` 里 `include "ma.h"` 调了 `ma_compute`，但 `risk.h` 里**没有任何 ma 的类型**——
用 risk 的人（app）不需要、不应该看见 ma。私有实现 → PRIVATE。
哪天 `risk.h` 里出现 `ma_config_t` 类型的参数，就必须改成 PUBLIC，
否则 app 编译时会报 `ma_config_t 未定义`——**报错信息会替你检查这个决定**。

**判定二：`target_include_directories(ma PUBLIC ...)` 为什么 PUBLIC？**
`ma.h` 是 ma 的对外接口，**链接 ma 的任何 target 都必须能 include 到它**。
删了 PUBLIC 改成 PRIVATE，立刻复现本章的"教科书错误"：

```text
app/main.c:2:10: fatal error: 'risk.h' file not found
    #include "risk.h"
             ^~~~~~~~
```

（app 链 risk，risk 的 include 路径是 PUBLIC 才传得过来；同理 risk 链 ma。）
这个错每个写 CMake 的人都会撞上一次——撞完就记住：
**头文件路径给谁用，就用对应的关键字**。

## 3.5 链接顺序与 undefined reference 怎么读

静态库时代遗留的经典报错：

```text
ld: undefined reference to `ma_compute'
```

读法分三步：①谁没定义——`ma_compute`，属于 ma 库；
②谁在找——报错上方的 `.o` 或库名；③为什么没找到——**链接器处理静态库是从左到右单趟扫描**：
命令行上 `librisk.a libma.a` 这个顺序才对（risk 在前，它引用的符号由后面的 ma 补上）；
顺序反了（`libma.a librisk.a`），扫描 ma 时还没有人引用它，整库被跳过，后面 risk 的引用就落空。

手写 Makefile 要自己排这个序；**CMake 里只要 `target_link_libraries` 声明对了依赖，
顺序由 CMake 按依赖图自动排**——又一件"手写会错、生成不会错"的事。

验证生成的链接命令（第 1 章的"怀疑生成物就打开它"习惯）：

```bash
cat build/app/CMakeFiles/feed_handler.dir/link.txt
# ... -o feed_handler  librisk.a  libma.a      ← 顺序正确，自动排出
```

## 3.6 本章验收清单

- [ ] 说得出 STATIC / SHARED 的核心区别（链接时机 + 部署形态），知道 HFT/嵌入式默认 STATIC
- [ ] 画出 demo/ 的依赖图，并指出每个 `target_link_libraries` / `target_include_directories` 的关键字及理由
- [ ] 亲手把 ma 的 PUBLIC 改成 PRIVATE，复现 `file not found`，再改回来
- [ ] undefined reference 的三步读法，知道静态库链接顺序为什么 CMake 不用手排
- [ ] 打开 `link.txt` 核对过自动生成的链接命令

## 3.7 与后续衔接

- **第 4 章** 解剖 `west build`：Zephyr 把本章的多目录组织放大到几百个模块——
  内核、驱动、协议栈全是 `add_subdirectory` 拉进来的库
- **第 5 章** 编译选项工程化：给不同 target 挂不同优化等级（行情库 -O3，工具代码 -O2），
  前提是这章的"一切皆 target"已经成本能
- 小练习：给 demo/ 再加一个 `libs/feed`（解析行情原始字节），让 risk 依赖它——
  体会"加一个模块 = 建一个目录 + 写 8 行 CMakeLists + 一行 add_subdirectory"的扩展成本

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
