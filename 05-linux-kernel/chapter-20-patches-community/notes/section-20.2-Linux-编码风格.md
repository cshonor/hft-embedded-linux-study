## ② Linux 编码风格 · Coding Style

大型项目 **统一风格** → 可读、少混乱、易协作。

#### 缩进

| 规则 | 说明 |
|------|------|
| **Tab = 8 字符宽度** | **禁止** 用 8 个空格代替 Tab |
| **`switch`** | **`case` 标签与 `switch` 对齐** |

#### 空格

| 有空格 | 无空格 |
|--------|--------|
| 关键字与 `(`：`if (`、`for (` | 函数名与 `(`：`foo(` |
| 多数二元/三元运算符两侧 | 一元运算符紧贴操作数：`*p`、`&x` |

#### 大括号 · K&R 风格

| 场景 | 风格 |
|------|------|
| 一般语句 | `{` 在 **行末** · `}` **独占一行** 作首字符 |
| **函数定义** | `{` **另起一行** |

```c
if (x) {
	do_one();
	do_two();
}

int foo(int x)
{
	return x;
}
```

#### 行长度

| 规则 | **≤ 80 字符** — 标准终端完整显示 |
|------|----------------------------------|
| 超长 | **手动折行** · 合理对齐续行 |

#### 命名

| 禁止 | **CamelCase** · **匈牙利命名** |
|------|-------------------------------|
| 推荐 | **小写 + 下划线** · 描述性全局名/函数名 |

#### 函数与注释

| 函数 | **短**（不超过一两屏）· **一事** · 局部变量 **≤ ~10** |
|------|--------------------------------------------------------|
| 注释 | 解释 **做什么、为什么** — 非复述 **怎么做** |

#### 其他建议

| 建议 | 原因 |
|------|------|
| **少用 `typedef`** | 隐藏真实类型 |
| **用内核已有例程** | 字符串、链表 — **勿造轮子**（Ch 6） |
| **少在 .c 里 `#ifdef`** | 尽量把头文件/配置交给编译系统 |
| **结构体初始化** | **C99 指定初始化**：`.field = val` |
| 遗留乱码 | **`indent`** 辅助格式化 |

→ **Documentation/process/coding-style.rst**（主线现行文档，比 3rd 书更全）

---

### 版本断崖：内核已经不是 C89 了

LKD（3rd / 本书）成书时内核用 `-std=gnu89`。**v5.18 起换成 `-std=gnu11`**
（v6.6 `Makefile:560` 实证）：

| 版本 | 编译标志 | 影响 |
|------|---------|------|
| **≤ v5.17** | `-std=gnu89` | 变量必须声明在块开头；`//` 注释理论上不合规 |
| **≥ v5.18** | `-std=gnu11` | 可以随处声明、混声明、`_Generic`、`static_assert` 等 |
| **v6.6** | 额外有 `-funsigned-char`、`-fno-strict-aliasing`、`-fno-common` | 见下 |

> **`-funsigned-char`（v6.6 `Makefile:562`）是最容易被忽略的一条**：
> 它把裸 `char` 的符号性在内核里**钉成无符号**。
> 这和用户态不一样（glibc + x86 上 `char` 默认 **signed**），
> 所以 [19.3](../../chapter-19-portability/notes/section-19.3-特定数据类型.md) 里
> 「`isalpha((unsigned char)c)`」那条纪律在**内核代码里不是必需的**（char 已无符号），
> 但在**用户态工具代码里依然是必需的**——同一个移植坑，两边结论不同，靠的是编译标志。

> 风格层面：混声明合法 ≠ 鼓励到处声明。内核普遍做法仍是
> **声明靠近首次使用、一个声明一行**（方便加注释、diff 干净），
> 而不是 C89 时代的"函数开头堆一排"。

---

### 行宽的真相：75 / 80 / 100 是三条不同的线

书里只说 80 列，实际有三档，**管的东西不一样**（v6.6 实证）：

| 数字 | 管什么 | 出处 |
|------|--------|------|
| **75** | **commit message 正文**（`COMMIT_LOG_LONG_LINE`：*"Prefer a maximum 75 chars per line"*） | `scripts/checkpatch.pl:3274` |
| **80** | **代码的"偏好上限"** —— 原文：*"The preferred limit on the length of a single line is 80 columns"*，且明确写了"**除非超过 80 列能显著提升可读性且不隐藏信息**" | `Documentation/process/coding-style.rst:104` |
| **100** | **checkpatch 真正告警的硬线**：`my $max_line_length = 100;`（`LONG_LINE` / `LONG_LINE_COMMENT` / `LONG_LINE_STRING`） | `scripts/checkpatch.pl:59` |

> ⚠️ 常见误传："内核已经把行宽放宽到 100 了。" —— **v6.6 的 coding-style.rst 里根本没有
> 100 列这句话**，它只说 80 是 *preferred*、可以超。
> 100 是 checkpatch 的实现常量。两者不矛盾：**文档管"应该"，脚本管"必须"。**

---

### 注释：写 WHAT / WHY，不写 HOW

`coding-style.rst` 第 8 节原文：

> *"NEVER try to explain HOW your code works in a comment: it's much better to
> write the code so that the **working** is obvious..."*

| 规则 | 说明 |
|------|------|
| 内容 | 说 **做什么**、**为什么**，不复读代码 |
| 位置 | 尽量放**函数头**；函数体里塞注释 = 该函数该拆了 |
| API 文档 | 用 **kernel-doc** 格式（`scripts/kernel-doc`，`W=1` 会检查） |
| 数据声明 | **一行一个**，给每个字段留注释位 |

多行注释有两种风格，**按目录区分**（v6.6 实证）：

```c
/* 通用风格（除 net/ 与 drivers/net/）
 * 左侧一整列星号，首尾各一个"几乎空白"的行。
 *
 * Description: 说明这个东西干什么。
 */

/* net/ 与 drivers/net/ 的专用风格
 * 差别只有一点：开头那一行不空，直接从内容开始。
 */
```

---

### `goto` 集中退出：这是有意的架构选择，不是坏味道

`coding-style.rst` 第 7 节给了四条理由（原文）：

| 理由 | 说明 |
|------|------|
| 无条件跳转**更容易读懂** | 控制流是线性的，不像嵌套 if 那样要人肉压栈 |
| **减少嵌套** | 少一层缩进，80 列更好用 |
| **不会漏改** | 新增资源时，不用去 N 个 return 分支各加一次清理 |
| 帮编译器省事 | 省掉优化重复清理代码的工作 |

```c
int fun(int a)
{
	int result = 0;
	char *buffer;

	buffer = kmalloc(SIZE, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;          /* 无需清理 → 直接 return */

	if (condition) {
		while (loop1) {
			...
		}
		result = 1;
		goto out_free_buffer;    /* 标签名说清「做什么」 */
	}
	...
out_free_buffer:
	kfree(buffer);
	return result;
}
```

> **两条细则**（原文强调）：
> ① 标签名要说明**做什么 / 为什么**（`out_free_buffer:` 好，`err1:` / `err2:` 差
> —— 增删退出路径时要重编号，且正确性难以人工核对）；
> ② **没有清理工作就别用 goto，直接 return。**

---

### `typedef` 禁令与它的四种例外

内核默认 **不给结构体和指针起 typedef**（"It's a mistake"）。允许的四种例外：

| 例外 | 例子 | 为什么允许 |
|------|------|-----------|
| **(a) 完全不透明对象** | `pte_t` | typedef 是**主动隐藏**内部结构，只能用访问函数操作；页表项在不同架构上没有任何可移植信息 |
| **(b) 清晰的整数类型** | `u8` / `u16` / `u32`（见 [19.2](../../chapter-19-portability/notes/section-19.2-字长和数据类型.md)） | 消歧义：不用猜它是 `int` 还是 `long`；但**必须有理由**——`unsigned long` 就别包装成 `myflags_t` |
| **(c) 用 sparse 造新类型做检查** | `__bitwise`、`__le32` | 这就是 [19.5 字节序](../../chapter-19-portability/notes/section-19.5-字节序.md) 里那套**零成本类型检查**的机制 |
| **(d) 与标准 C99 类型相同的情形** | 部分整数包装 | 属于 (b) 的延伸 |

> 判据很简单：**"有没有理由？"** 没有理由的 typedef 一律不写。
> 这条纪律和 HFT 代码里"给价格/数量定义强类型"**不矛盾**——
> 后者属于 (b)(c)：有明确位宽语义、且想让编译器帮你抓错。

---

### 宏的五个陷阱（原文列举）

| # | 陷阱 | 典型翻车 |
|---|------|---------|
| 1 | **影响控制流的宏** | `return` / `goto` 藏在宏里，调用处看不出来 |
| 2 | **依赖"魔法"局部变量名** | 宏体内直接用 `ret` 之类的名字，与调用者变量冲突 |
| 3 | **参数当左值** | `FOO(x) = y;` 这种宏，语义极难维护 |
| 4 | **常量表达式忘加括号** | `#define CONSTEXP (CONSTANT \| 3)` —— 括号是必须的 |
| 5 | **类函数宏里的局部命名冲突** | 用 `({ ... })` 语句表达式时要防变量名撞车 |

---

### 自动检查工具链：不止 `checkpatch.pl`

| 工具 | 怎么跑 | 抓什么 |
|------|--------|--------|
| **`checkpatch.pl`** | `scripts/checkpatch.pl --strict file.patch` | 风格、签名格式、`Fixes:` 写法、行宽 |
| **`sparse`** | `make C=1`（改动的）/ `C=2`（全部） | 类型检查：`__user` / `__bitwise` / 地址空间、端序混用（呼应 19.5） |
| **`smatch`** | 外部工具 | 流程分析：空指针、锁状态、错误码路径 |
| **`coccinelle`** | `make coccicheck` | API 演进的批量改写（`make coccicheck MODE=patch`） |
| **`kernel-doc`** | `make W=1` | 文档注释与函数签名不一致 |
| **`rustfmt` / `clippy`** | Rust 部分（`rust/`） | v6.6 起内核有 Rust 代码 |

> **最重要的一条态度**（`5.Posting` 原文）：
> *"checkpatch.pl, while being the embodiment of a fair amount of thought
> about what kernel patches should look like, is not smarter than you.
> If fixing a checkpatch.pl complaint would make the code worse, don't do it."*
> —— 工具是提示，不是法官。

---

### HFT 视角：这些规则哪些值得照搬

| 规则 | 在 HFT 代码里的对应价值 |
|------|----------------------|
| **80 列偏好** | 分屏 diff / 并排 review 时不用横向滚动；`git log -p` 在终端里完整可读 |
| **goto 集中退出** | 下单/撤单路径有多个失败点时，清理逻辑集中在一处 = **不会漏掉回滚某一步**，这是正确性不是风格 |
| **注释写 WHY** | 低延迟代码里最值钱的注释是"**为什么这里必须无锁 / 为什么不能用 std::map**"，而不是"这里在取队首" |
| **typedef 必须有理由** | 呼应 19.2/19.5：`price_t` / `qty_t` 这类**有理由**的强类型照搬；`typedef unsigned long flags_t` 这种不要 |
| **一行一个数据声明** | 便于给每个字段写"单位 / 精度 / 取值范围"——定点价格字段的注释位置就是钱 |
| **工具链** | HFT 项目同样应该把 `-Wconversion`、`clang-tidy`、`sparse` 风格的检查挂进 CI，把"类型纪律"交给机器 |

<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 内核代码风格的关键规则有哪些？为什么强制？

<details><summary>答案</summary>

1) 缩进用 Tab（8 字符宽）不是空格；2) 行宽 80 字符；3) 函数 < 50 行；4) 变量声明在块开头（C89 风格）；5) `goto` 集中错误处理。强制统一风格是因为内核有千万行代码、数千贡献者，统一风格降低 review 成本。`scripts/checkpatch.pl` 自动检查。HFT 团队代码也应建立类似规范。

> **⚠️ 按 v6.6 修订其中两条**：
> ① 第 4 条"C89 风格、声明必须在块开头"——**已过时**。内核自 **v5.18 起用 `-std=gnu11`**
> （v5.17 还是 `gnu89`），混声明完全合法；现在的做法是"声明靠近首次使用、一行一个"，
> 理由是便于加注释和保持 diff 干净，而不是语言限制。
> ② 第 2 条"80 字符"——准确说法是**三档**：commit message **75**（checkpatch 提示）、
> 代码**偏好 80**（coding-style.rst 明文允许为可读性超出）、checkpatch 硬告警线 **100**。

</details>

**Q2.** 内核为什么反 `typedef`？什么情况下又允许？

<details><summary>答案</summary>

默认禁止，因为 typedef 会**隐藏真实类型**——看到 `vps_t` 你不知道它是指针、结构体还是整数，而看到 `struct foo *` 一目了然。四种例外：(a) 完全不透明对象（`pte_t` 这类只能靠访问函数操作、跨架构无共同信息的）；(b) 清晰整数类型（`u8/u16/u32`，消除 int/long 歧义，但**必须有理由**）；(c) 用 sparse 造新类型做检查（`__bitwise`、端序类型）；(d) 与标准 C99 类型相同的情形。判据是"有没有理由"——没理由的一律不写。

</details>

**Q3.** 内核用 `goto` 做集中退出合理吗？写出它的四条理由和两条细则。

<details><summary>答案</summary>

合理，且是文档明写的模式（`coding-style.rst` 第 7 节）。四条理由：① 无条件跳转比嵌套分支更容易读懂；② 减少嵌套层级（也顺带缓解 80 列压力）；③ 新增资源时不会漏改某个 return 分支的清理；④ 省掉编译器优化重复清理代码的工作。两条细则：① 标签名要说明"做什么/为什么"（`out_free_buffer:` 好，`err1:`/`err2:` 差，因为增删退出路径要重编号、正确性难核对）；② 没有清理工作就别用 goto，直接 return。

</details>

**Q4.** 内核里的 `char` 是有符号还是无符号？为什么这点和用户态不一样？

<details><summary>答案</summary>

**无符号**。v6.6 `Makefile:562` 里有 `-funsigned-char`，把裸 `char` 的符号性在内核内固定为 unsigned。用户态不同：glibc + x86 上 `char` 默认 signed，所以标准库要求 `isalpha((unsigned char)c)` 这样显式转换（传负数进 ctype 是 UB，glibc 表现为跳表越界读）。同一个移植陷阱，内核侧被编译标志消除了，用户态工具代码仍然存在——写代码时要分清自己在哪一侧。

</details>

</details>
---
