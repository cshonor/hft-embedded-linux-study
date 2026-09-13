# 2.7 Debug 构建 vs Release 构建（release 模式是什么、为什么"只有 release 才崩"）

> 🔴 精读 · 用户态正确性调试

## 本节要点

2.1 讲的是「怎么编译出**能调试**的二进制」；这一节讲它的对面：**线上那个 release 二进制到底是怎么回事**，以及为什么同一份源码、同一台机器，`-O0` 下怎么测都对、一上生产就崩。

一句话答案：**「release 模式」不是编译器的开关，也不是 C 标准里的概念**——它是"为发布而调"的一套**构建配置约定**（build configuration），由 IDE / 构建系统 / 发行版各定义一套；gcc 根本没有 `--release` 这个选项。它真正危险的地方在于：**它会静默改掉程序的语义**（断言消失、UB 被"利用"、NULL 检查被删）。

所以听到「release 模式」，应该追问三件事：**谁的 release？带不带 `-g`？`NDEBUG` 定没定？**

## 一、没有任何标准定义过「release 模式」

| 来源 | 有"release 模式"这个概念吗 |
|------|-----------------------------|
| C 标准（C11/C17/C23） | ❌ 没有。唯一被标准化的是 **`NDEBUG`**（控制 `assert`） |
| POSIX | ❌ 没有 |
| GCC / Clang | ❌ 没有 `--release` 选项，只有一堆 `-O` / `-g` / `-D` 旗标 |
| IDE（MSVC） | ✅ 有：项目属性里的 Debug / Release 下拉框 |
| CMake | ✅ 有：`CMAKE_BUILD_TYPE` = `Debug` / `Release` / `RelWithDebInfo` / `MinSizeRel` |
| Cargo（Rust） | ✅ 有，而且是**语言级**的：`[profile.release]` 直接改变运行时语义 |

Cargo 官方把话讲得最直白（Cargo Book · Profiles）：

> The `release` profile is intended for optimized artifacts used for releases and in production.

—— "为**发布**和**生产环境**准备的、已优化的产物"。这就是 release 模式的一般含义：**优化打开、断言关掉**；而"调试信息关掉、符号剥掉"只是**常见做法**、不是定义的一部分（CMake 的 `Release` 并不 strip；Cargo 的 `strip` 默认也是 `"none"`）。

## 二、四个官方配置到底差在哪

CMake 3.28 给 GCC/Clang 定的默认旗标（`Modules/Compiler/GNU.cmake:103-106` 逐字）：

```cmake
string(APPEND CMAKE_${lang}_FLAGS_DEBUG_INIT          " -g")
string(APPEND CMAKE_${lang}_FLAGS_MINSIZEREL_INIT     " -Os -DNDEBUG")
string(APPEND CMAKE_${lang}_FLAGS_RELEASE_INIT        " -O3 -DNDEBUG")
string(APPEND CMAKE_${lang}_FLAGS_RELWITHDEBINFO_INIT " -O2 -g -DNDEBUG")
```

MSVC 侧（`Modules/Platform/Windows-MSVC.cmake:539-542` 逐字，`_MDd`=`/MDd`、`_MD`=`/MD`、`_Zi`=`/Zi`、`_RTC1_local`=`/RTC1`）：

```cmake
CMAKE_${lang}_FLAGS_DEBUG_INIT          "${_MDd}${_Zi} /Ob0 /Od${_RTC1_local}"
CMAKE_${lang}_FLAGS_RELEASE_INIT        "${_MD} /O2 /Ob2 /DNDEBUG"
CMAKE_${lang}_FLAGS_RELWITHDEBINFO_INIT "${_MD}${_Zi} /O2 /Ob1 /DNDEBUG"
CMAKE_${lang}_FLAGS_MINSIZEREL_INIT     "${_MD} /O1 /Ob1 /DNDEBUG"
```

汇总：

| 配置 | 优化 | 调试信息 | `assert` | 备注 |
|------|------|----------|----------|------|
| `Debug` | `-O0` | `-g` | 生效 | 只有它带 `-g`；MSVC 额外 `/RTC1` 运行期检查 |
| `Release` | `-O3` | 无 | 关 | MSVC `/O2 /Ob2` |
| `RelWithDebInfo` | `-O2` | `-g` | 关 | **工程上最实用**：能优化还能回溯 |
| `MinSizeRel` | `-Os` | 无 | 关 | 嵌入式只读文件系统常用 |

Rust 把同样的思想做进了语言（Cargo Book · Default profiles 逐字）：

| 设置 | `[profile.dev]` | `[profile.release]` |
|------|-----------------|---------------------|
| `opt-level` | `0` | `3` |
| `debug` | `true` | `false` |
| `debug-assertions` | `true` | `false` |
| `overflow-checks` | `true` | `false` |
| `incremental` | `true` | `false` |

⚠️ **四点必看**：

1. 除 `Debug` 外**三个配置都带 `-DNDEBUG`**——"release 模式"有一半含义就是"**关断言**"。
2. **只有 `Debug` 带 `-g`**。想"又能优化又能回溯"→ `RelWithDebInfo`。
3. MSVC 换了 CRT 变体：`/MDd`（Debug CRT）→ `/MD`（Release CRT）。**两者不能混链**，否则链接期报 CRT 不匹配（LNK2038 一类）。这是 Windows 上"Debug 好、Release 崩"最常见的一种成因。
4. Rust 还有一条 C 没有的：`overflow-checks` 在 release 下**关掉**，整数溢出从 panic 变成静默回绕（[Rust 3.2](../../../17-rust-foundation/00-Book/03-common-concepts/3.2-数据类型.md)）。

## 三、Release 到底改了什么（每条都配实测）

> 以下均为 **CE 复核**（Compiler Explorer · gcc 13.3 · x86-64）：拿到的是真实编译诊断与真实运行输出，但**地址与耗时来自另一台机器**，不作本机数据。

### 3.1 `NDEBUG`：`assert` 整行消失，连参数副作用一起带走

C 标准原文（`<assert.h>`，C11 §7.2.1.1 / C23 §7.2.2.1）：

```c
#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define assert(condition) /*implementation defined*/
#endif
```

> If `NDEBUG` is defined as a macro name at the point in the source code where `<assert.h>` is included, then `assert` does nothing.

`((void)0)` 的含义是**参数表达式根本不会被求值**——所以 `assert(next_seq() == 1)` 在 release 里连 `next_seq()` 都不会调用。

CE 复核（源码 `rel_e1.c`）：

```c
static int seq = 0;
static int next_seq(void) { return ++seq; }
static int handle(int qty) { assert(qty > 0); return qty * 2; }

int main(void) {
    assert(next_seq() == 1);            /* 本意：自增序号 + 顺手检查 */
    printf("A: seq=%d\n", seq);
    printf("B: handle(-5) = %d\n", handle(-5));   /* 本意：非法入参应被拦下 */
    return 0;
}
```

| 编译旗标 | 实际输出 |
|----------|----------|
| `-O0 -Wall`（无 `NDEBUG`） | `A: seq=1` → 随后 `Assertion 'qty > 0' failed.` / `SIGABRT` |
| `-O2 -DNDEBUG -Wall` | `A: seq=0`、`B: handle(-5) = -10`（**负数被静默放行**） |

编译器还顺手给了铁证：

```
warning: 'next_seq' defined but not used [-Wunused-function]
```

—— 那整行被删到"函数成了死代码"，编译器只好抱怨一句。

**工程含义**：

- `assert` 是**调试期校验**，不是校验。订单合法性、账户余额、越界保护必须用真 `if` + 错误返回，或自定义常开宏（HFT 里常见的 `ASSERT_ALWAYS` / `RISK_CHECK`）。
- 副作用（自增序号、计数、`malloc`、`close`）绝不能只写在 `assert` 里。同类坑在宏参数上已出现过一次，见 [14.2.4 宏参数的副作用](../../../01-c-language/02-advanced-pointers-and-memory/ch14-preprocessor/14.2-define/14.2.4-宏参数的副作用.md)。
- 注意 `NDEBUG` **不是标准库定义的**（cppreference 原文：*"which is not defined by the standard library"*）——它必须由构建系统给你 `-DNDEBUG`。这也解释了为什么它总出现在构建旗标里。

### 3.2 `-O2` 一开，UB 就从"能跑"变成"乱跑"

`-O2` 默认开启的旗标里，有几个**直接和内存/别名语义相关**（GCC 手册 `-O2` 列表逐字）：

```
-fstrict-aliasing   -fdelete-null-pointer-checks   -fisolate-erroneous-paths-dereference
-finline-functions  -foptimize-sibling-calls       -ftree-loop-vectorize  ...
```

**实测 A：严格别名（`rel_e4.c`）**——union 里用 `int*` 与 `float*` 访问同一块内存：

```c
static int type_pun(int *pi, float *pf) {
    *pi = 0;
    *pf = 1.0f;      /* 编译器若认为 pi/pf 不可能别名，这次写入就是"无效"的 */
    return *pi;
}
```

| 旗标 | `type_pun(&u.i, &u.f)` 返回 |
|------|------------------------------|
| `-O0 -Wall` | `1065353216`（= `0x3F800000`，`1.0f` 的位模式）——"对" |
| `-O2 -Wall` | `0` —— 认定二者不可能别名，`*pf = 1.0f` 对 `*pi` 无影响 |
| `-O2 -fno-strict-aliasing -Wall` | `1065353216` —— 关掉别名优化后恢复 |

这就是教科书级的「**Debug 跑得对、Release 跑得错**」。注意它是**未定义行为**，不是编译器 bug。合法替代：`memcpy`（`char*` 别名是标准明文允许的例外）。

**实测 B：先解引用之后的 NULL 检查被删（`rel_e5.c`）**

```c
int deref_or_default(int *p) {
    int v = *p;                 /* 先解引用 */
    if (p == NULL) return -1;   /* UB：既然已解引用，编译器认为 p 不可能是 NULL */
    return v;
}
```

| 旗标 | 反汇编（CE 复核） |
|------|-------------------|
| `-O0` | `mov rax, QWORD PTR [rbp-24]` / `mov eax, [rax]` / `cmp QWORD PTR [rbp-24], 0` / `jne .L2` —— 检查在 |
| `-O2` | `mov eax, DWORD PTR [rdi]` / `ret` —— **检查整段消失** |

**实测 C（反直觉，重点记）**：有符号溢出的 UB **连 `-O0` 都不可信**。

```c
static int always_true(int i) { return i + 1 > i; }   /* 数学上恒真 */
```

| 旗标 | `always_true(INT_MAX)` |
|------|------------------------|
| `-O0 -Wall` | `1` ← **也在按"不会溢出"折叠** |
| `-O2 -Wall` | `1` |
| `-O2 -fwrapv -Wall` | `0` ← 真去算了：回绕成 `INT_MIN > INT_MAX` |

GNU C 手册（Signed Overflow）原文：

> For signed integers, the result of overflow in C is in principle undefined ... For instance, `int i; ... if (i < i + 1) x = 5;` could be optimized to do the assignment unconditionally.

**结论**：别以为"用 `-O0` 编译就安全"。UB 的利用**不挑优化等级**，前端折叠阶段就会动手。三条解药：

1. 用无符号类型 / 明确宽度做回绕；
2. 确实依赖回绕语义的模块显式加 `-fwrapv`（GCC 手册原句：*"instructs the compiler to assume that signed arithmetic overflow of addition, subtraction and multiplication wraps around using twos-complement representation"*）；
3. 测试期用 `-fsanitize=signed-integer-overflow` 抓现行（见 [3.3 UBSan](../../chapter-03-memory/notes/03-undefinedbehaviorsanitizer.md)）。

### 3.3 `-g` 与优化正交——别把符号当"Debug 专属"

CE 复核：`asm(-O2)` 与 `asm(-O2 -g)` **逐字节相同**。加 `-g` 不会改一行机器码，代价只有二进制体积。

GCC 手册（Debugging Options）明确允许混用，也诚实交代代价：

> GCC allows you to use `-g` with `-O`. The shortcuts taken by optimized code may occasionally be surprising: some variables you declared may not exist at all; flow of control may briefly move where you did not expect it; ...

所以"生产二进制不带 `-g`"**不是技术限制，而是体积与防逆向的取舍**——于是有了 §4 的 `RelWithDebInfo` 与 §5 的符号归档。

顺手一个实用选项 `-Og`（手册原文）：

> With no `-O` option at all, some compiler passes that collect information useful for debugging do not run at all, so that `-Og` may result in a better debugging experience.

—— "要调试但嫌 `-O0` 太慢、又不想被 `-O2` 折腾"时，用 `-Og -g`。

### 3.4 帧指针省略 + 内联：栈回溯变难

GCC 手册原文：

> `-fomit-frame-pointer` ... **Enabled by default at `-O1` and higher.**

CE 复核（`rel_e3.c`）：

| 旗标 | 帧指针 `rbp` |
|------|---------------|
| `-O0` | 有（`push rbp` / `mov rbp, rsp`） |
| `-O2` | 消失 |
| `-O2 -fno-omit-frame-pointer` | **仍然消失** |

最后一行不是实验失败——手册同一条的下半句正好解释它：

> Note that `-fno-omit-frame-pointer` doesn't guarantee the frame pointer is used in all functions. Several targets always omit the frame pointer in leaf functions.

（`add2` 被优化成叶子函数、不需要栈帧，于是连"强制"也省了。）

后果与对策：

- `bt` 现在靠 DWARF 的 **CFI 展开表**（`.eh_frame`）还能走，**不依赖 `rbp`**；但内联进来的帧会消失、尾调用（`-foptimize-sibling-calls`）会让调用者从栈里"蒸发"，现象就是**帧数比源码少**。
- **perf 采样要单独加 `-fno-omit-frame-pointer`**（见 [Ch6 性能类](../../chapter-06-performance/README.md)），否则火焰图只能靠 DWARF 展开，更慢更浅。

### 3.5 加固旗标是**发行版替你加的**

Ubuntu 默认加固旗标（Ubuntu Wiki `ToolChain/CompilerFlags` + gcc-13 发行版补丁）：

| 旗标 | Ubuntu 从哪版起默认 |
|------|---------------------|
| `-fstack-protector-strong` | 14.10 起（6.10 起先是 `-fstack-protector`） |
| `-D_FORTIFY_SOURCE=3` | **24.04 起**（8.10–23.10 为 `=2`） |
| `-fPIE` | 16.04 起 |
| `-fcf-protection` | 19.10 起 |
| `-fstack-clash-protection` | 19.10 起 |
| `-Wl,-z,relro` / `-Wl,-z,now` | 8.10 起 |

可以自己验证，`gcc -dumpspecs` 里就写着：

```
%{!O0:%{O*:%{!D_FORTIFY_SOURCE=*:%{!U_FORTIFY_SOURCE:-D_FORTIFY_SOURCE=3}}}}
```

读法：**没给 `-O0`、给了 `-O*`、你没自己定义 `_FORTIFY_SOURCE`、也没 `-U` 掉它 → 自动补 `=3`**。
即 Ubuntu 24.04 上 `gcc -O2 x.c` 等价于 `gcc -O2 -D_FORTIFY_SOURCE=3 x.c`。glibc 手册也讲了它的前提：

> If `_FORTIFY_SOURCE` is set to 1, with compiler optimization level 1 (gcc `-O1`) and above, checks that shouldn't change the behavior of conforming programs are performed.

**这才是"release 才崩"的另一个来源**：`-O0` 下 `_FORTIFY_SOURCE` 根本不激活，`-O2` 下激活了，某些越界调用就从"默默越界"变成 `*** buffer overflow detected ***` + `abort()`。在 `bt` 里看到这些**别当成 bug**：

| 你在 `bt` 里看到 | 含义 |
|------------------|------|
| `__memcpy_chk` / `__strcpy_chk` / `__printf_chk` | 被 `_FORTIFY_SOURCE` 换成了带长度检查的版本 |
| `*** stack smashing detected ***` | `-fstack-protector-strong` 的栈金丝雀被打翻 → 确实有栈溢出 |
| 栈顶是 `__stack_chk_fail` | 同上，回去找**返回地址被改写**的位置 |

⚠️ `_FORTIFY_SOURCE` 必须在**头文件之前**由命令行定义（写在源文件里 `#define` 太晚）——这也是它总出现在构建旗标、而不是代码里的原因。

## 四、该用哪个配置：决策表

| 场景 | 推荐配置 | 旗标（GCC/Clang） |
|------|----------|-------------------|
| 单步 / 看变量 | `Debug` | `-O0 -g`（或 `-Og -g`） |
| CI 回归测试 | **`Release` 必须也跑一遍** | `-O2/-O3 -DNDEBUG` |
| 上生产 / 压测 | `RelWithDebInfo` | `-O2 -g -DNDEBUG` |
| 极限性能 | `Release` + 另存符号 | `-O3 -march=native -flto`（`-g` 可选） |
| 抓 UB / 越界 | Sanitizer 组合 | `-O1 -g -fsanitize=address,undefined` |
| 抓竞态 | TSan | `-O1 -g -fsanitize=thread` |

（Sanitizer 的用法与限制见 [3.2 ASan](../../chapter-03-memory/notes/02-addresssanitizer.md) / [3.3 UBSan](../../chapter-03-memory/notes/03-undefinedbehaviorsanitizer.md) / [4.3 TSan](../../chapter-04-concurrency/notes/03-threadsanitizer.md)。）

## 五、HFT 工程纪律

1. **生产二进制必须能回溯**：用 `RelWithDebInfo`，或 `Release` + 分离符号——`objcopy --only-keep-debug prog prog.debug` 再 `objcopy --add-gnu-debuglink=prog.debug prog`，并把符号文件按 build-id 归档进符号服务器。没有符号的 core 就是一堆十六进制（见 [2.5 加载 core 回溯](05-load-core-backtrace.md)）。
2. **测试矩阵里必须有优化构建**。§3.2 的别名实验说明：`-O0` 全绿不代表 `-O2` 全绿。
3. **关键校验不用 `assert`**（风控、下单数量、账户余额），用常开宏。`assert` 在 release 里等于删行。
4. **序号/计数用无符号或显式回绕**；确实依赖回绕的模块单独加 `-fwrapv`。
5. **CMake 里把 `CMAKE_BUILD_TYPE` 与旗标写死**，别依赖 IDE 下拉框或发行版默认——§3.5 那些旗标会在你不知情时改变崩溃行为。
6. **"Debug 过、Release 崩"＝优先怀疑 UB**，不要先怀疑编译器。用 `-fsanitize=undefined` 在同一份输入上复现。

---

<details>
<summary>自测题（点击展开，6 问）</summary>

**Q1:** 「release 模式」是 C 标准里的概念吗？如果不是，它指什么？唯一被标准化的开关是哪个？

> 不是。标准 / POSIX / GCC / Clang 都没有"release 模式"这个概念（gcc 没有 `--release`）。它是**构建配置约定**：IDE（MSVC Debug/Release）、CMake（`CMAKE_BUILD_TYPE`）、Cargo（`[profile.release]`）各定义一套，含义是"优化打开 + 通常不带 `-g` + 断言被 `-DNDEBUG` 关掉"。唯一被标准规定的是 `NDEBUG`（`<assert.h>`，C11 §7.2.1.1），且它**不由标准库定义**，必须由构建系统 `-DNDEBUG` 传进来。

**Q2:** `assert(next_seq() == 1)` 在 `-DNDEBUG` 下会发生什么？为什么这对 HFT 是严重风险？

> 整行变成 `((void)0)`，**`next_seq()` 根本不会被调用**——副作用一起消失。CE 实测：无 `NDEBUG` 时 `seq=1`、随后 `assert(qty>0)` 触发 `SIGABRT`；`-O2 -DNDEBUG` 时 `seq=0`、`handle(-5)` 静默返回 `-10`，编译器还报 `'next_seq' defined but not used` 作为铁证。风险：序号跳号、非法输入（负数下单量）被静默放行。

**Q3:** 用 `-O0` 编译的程序就"安全"了吗？举一个实测例子。

> 不安全。CE 复核：`int always_true(int i){ return i + 1 > i; }` 对 `INT_MAX` 在 `-O0` 下**也**返回 `1`（前端就按"不会溢出"折叠了），`-O2` 同样；只有加 `-fwrapv` 才返回 `0`。有符号溢出是 UB，编译器利用 UB 不受优化等级限制。

**Q4:** 同一份源码 `-O0` 返回 `1065353216`、`-O2` 返回 `0`，最可能是什么问题？怎么验证？

> 严格别名（strict aliasing）：`-O2` 默认开 `-fstrict-aliasing`，编译器假定 `int*` 与 `float*` 不指向同一对象，于是"写 float 后读 int"被当作无关联，直接返回旧值。验证：加 `-fno-strict-aliasing` 重跑，若结果恢复成 `1065353216` 即确认。根治写法是 `memcpy`。

**Q5:** 为什么生产二进制可以既 `-O2` 又 `-g`？加 `-g` 会拖慢运行吗？

> 不会。`-g` 只往 ELF 里写 DWARF（`.debug_*` 段），**不参与代码生成**——CE 复核 `asm(-O2)` 与 `asm(-O2 -g)` 逐字节相同。GCC 手册也允许 `-g` 配 `-O`，只提醒优化后调试体验会"surprising"。代价只有体积与信息泄露（DWARF 里有源码路径、函数名、类型布局，见 [2.1](01-gdb-intro-build.md) Q5）。

**Q6:** 为什么 Ubuntu 24.04 上 `-O0` 能跑的越界程序，用 `-O2` 编出来会 `SIGABRT`？

> 因为 `_FORTIFY_SOURCE` 需要优化才生效。Ubuntu 24.04 的 gcc 默认（`gcc -dumpspecs` 可见 `%{!O0:%{O*:...-D_FORTIFY_SOURCE=3}}`）在**非 `-O0`** 且有 `-O*` 时自动补 `=3`。`-O0` 下检查不激活，`-O2` 下激活 → 越界调用被 `__*_chk` 拦下并 `abort()`。

</details>

## 交叉引用

- [2.0 GDB 总览：能干什么、不能干什么](00-gdb-overview.md)
- [2.1 gdb 入门与调试信息（-g 编译 / debuginfo / 加载方式）](01-gdb-intro-build.md) —— `-g` / DWARF / stripped 的基础
- [2.5 加载 core 回溯](05-load-core-backtrace.md) —— §5 的"符号 + build-id 归档"就是为它服务
- [3.2 ASan](../../chapter-03-memory/notes/02-addresssanitizer.md) / [3.3 UBSan](../../chapter-03-memory/notes/03-undefinedbehaviorsanitizer.md) / [4.3 TSan](../../chapter-04-concurrency/notes/03-threadsanitizer.md)
- [Ch6 性能类：热点采样](../../chapter-06-performance/README.md) —— `-fno-omit-frame-pointer` 与火焰图
- [14.2.4 宏参数的副作用](../../../01-c-language/02-advanced-pointers-and-memory/ch14-preprocessor/14.2-define/14.2.4-宏参数的副作用.md) —— 同一类"被关掉的宏带走副作用"
- [Rust 3.2 数据类型](../../../17-rust-foundation/00-Book/03-common-concepts/3.2-数据类型.md) —— Rust 把 debug/release 差异做进了语言（`overflow-checks`）
