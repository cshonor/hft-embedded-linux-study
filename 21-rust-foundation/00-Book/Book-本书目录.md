# The Book — 本书目录

> **书名**：《Rust 程序设计语言》（*The Rust Programming Language*，俗称 **The Book**）  
> **在线全文**：[doc.rust-lang.org/book](https://doc.rust-lang.org/book/)（中文版：[rustwiki.org/zh-CN/book](https://rustwiki.org/zh-CN/book/)）  
> **本仓库**：笔记与 demo 在 **`00-Book/01-*`～`00-Book/19-*`**（与本文件同目录）；本文为**全书导航索引**。

**扩展阅读**：[Effective Rust](../01-ER/ER-本书目录.md)（`01-ER/`，35 条建议，与主线并行）

---

## 使用说明

| 规则 | 说明 |
|------|------|
| 笔记路径 | `00-Book/NN-topic/节号-标题.md`（或相对本目录 `./NN-topic/...`） |
| demo | 第 3 章起多为 `节号-英文名-demo/`，与对应 `.md` 同级 |
| 学习清单 | 勾选进度见根目录 [`Rust学习笔记.md`](../Rust学习笔记.md) |

---

## 章节索引

| 章 | 主题 | 本地目录 |
|----|------|----------|
| 1 | 入门指南 | [01-getting-started](./01-getting-started/) |
| 2 | 猜数字游戏 | [02-guessing-game](./02-guessing-game/) |
| 3 | 通用编程概念 | [03-common-concepts](./03-common-concepts/) |
| 4 | 认识所有权 | [04-ownership](./04-ownership/) |
| 5 | 结构体 | [05-structs](./05-structs/) |
| 6 | 枚举与模式匹配 | [06-enums-pattern-matching](./06-enums-pattern-matching/) |
| 7 | 包、Crate 与模块 | [07-packages-modules](./07-packages-modules/) |
| 8 | 常见集合 | [08-collections](./08-collections/) |
| 9 | 错误处理 | [09-error-handling](./09-error-handling/) |
| 10 | 泛型、Trait 与生命周期 | [10-generics-traits-lifetimes](./10-generics-traits-lifetimes/) |
| 11 | 自动化测试 | [11-testing](./11-testing/) |
| 12 | I/O 项目：命令行程序 | [12-cli-project](./12-cli-project/) |
| 13 | 迭代器与闭包 | [13-iterators-closures](./13-iterators-closures/) |
| 14 | Cargo 与 Crates.io | [14-cargo-crates](./14-cargo-crates/) |
| 15 | 智能指针 | [15-smart-pointers](./15-smart-pointers/) |
| 16 | 无畏并发 | [16-fearless-concurrency](./16-fearless-concurrency/) |
| 17 | 面向对象特性 | [17-oop](./17-oop/) |
| 18 | 模式 | [18-patterns](./18-patterns/) |
| 19 | 高级特性 | [19-advanced-features](./19-advanced-features/) |

---

## 第 1 章 — 入门指南

| 节 | 笔记 | demo |
|----|------|------|
| 1.1 安装 | [1.1-安装.md](./01-getting-started/1.1-安装.md) | — |
| 1.2 Hello, World! | [1.2-Hello-World.md](./01-getting-started/1.2-Hello-World.md) | [hello_world](./01-getting-started/hello_world/) |
| 1.3 Hello, Cargo! | [1.3-Hello-Cargo.md](./01-getting-started/1.3-Hello-Cargo.md) | [hello cargo](./01-getting-started/hello%20cargo/) |

---

## 第 2 章 — 猜数字游戏

| 节 | 笔记 / 项目 |
|----|-------------|
| 2 实战 | [02-guessing-game/README.md](./02-guessing-game/README.md) · 项目根目录 [02-guessing-game](./02-guessing-game/) |

---

## 第 3 章 — 通用编程概念

| 节 | 笔记 | demo |
|----|------|------|
| 3.1 变量和可变性 | [3.1-变量和可变性.md](./03-common-concepts/3.1-变量和可变性.md) | [3.1-variables-demo](./03-common-concepts/3.1-variables-demo/) |
| 3.1 延伸 | [下划线占位符 `_`](./03-common-concepts/3.1-下划线占位符.md) | — |
| 3.2 数据类型 | [3.2-数据类型.md](./03-common-concepts/3.2-数据类型.md) | [3.2-data-types-demo](./03-common-concepts/3.2-data-types-demo/) |
| 3.2 延伸 | [JSON 与编码](./03-common-concepts/3.2-JSON与编码.md) · [Unicode 与字符串](./03-common-concepts/3.2-Unicode与字符串.md) | [json-encoding](./03-common-concepts/3.2-json-encoding-demo/) · [unicode](./03-common-concepts/3.2-unicode-demo/) |
| 3.3 函数 | [3.3-函数.md](./03-common-concepts/3.3-函数.md) | [3.3-functions-demo](./03-common-concepts/3.3-functions-demo/) |
| 3.4 注释 | [3.4-注释.md](./03-common-concepts/3.4-注释.md) | [3.4-comments-demo](./03-common-concepts/3.4-comments-demo/) |
| 3.5 控制流 | [3.5-控制流.md](./03-common-concepts/3.5-控制流.md) | [3.5-control-flow-demo](./03-common-concepts/3.5-control-flow-demo/) |

---

## 第 4 章 — 认识所有权

| 节 | 笔记 | demo |
|----|------|------|
| 4.1 所有权 | [4.1-什么是所有权.md](./04-ownership/4.1-什么是所有权.md) | [4.1-ownership-demo](./04-ownership/4.1-ownership-demo/) |
| 4.1 延伸 | [const 与 static](./04-ownership/4.1-const与static.md) | — |
| 4.2 引用与借用 | [4.2-引用与借用.md](./04-ownership/4.2-引用与借用.md) | [4.2-references-demo](./04-ownership/4.2-references-demo/) |
| 4.3 切片 | [4.3-切片slice.md](./04-ownership/4.3-切片slice.md) | [4.3-slices-demo](./04-ownership/4.3-slices-demo/) |
| 4.3.1 | [usize 下标与悬空索引](./04-ownership/4.3.1-usize下标与悬空索引.md) | ↑ 同上 demo |
| 4.3.2 | [切片与借用规则（& / &mut）](./04-ownership/4.3.2-切片与借用规则.md) | ↑ |
| 4.3.3 | [str 与 &str 类型](./04-ownership/4.3.3-str与str类型.md) | ↑ |
| 4.3.4 | [DST 与 Sized](./04-ownership/4.3.4-DST与Sized.md) | ↑ |

---

## 第 5 章 — 使用结构体组织关联数据

| 节 | 笔记 | demo |
|----|------|------|
| 5.1 定义并实例化结构体 | [5.1](./05-structs/5.1-定义并实例化结构体.md) | [5.1-structs-demo](./05-structs/5.1-structs-demo/) |
| 5.1.1 | [修改字段与可变性](./05-structs/5.1.1-修改字段与可变性.md) | ↑ 同上 demo |
| 5.1.2 | [结构体更新语法 `..orig`](./05-structs/5.1.2-结构体更新语法.md) | ↑ |
| 5.1.3 | [字段所有权 String/&str](./05-structs/5.1.3-字段所有权与引用.md) | ↑ |
| 5.1.4 | [Debug/Display 与 Trait 约束](./05-structs/5.1.4-Debug-Display与Trait约束.md) | ↑ |
| 5.1.5 | [结构体实体 vs 引用 vs Box](./05-structs/5.1.5-结构体变量实体与引用.md) | ↑ |
| 5.2 结构体示例 | [5.2](./05-structs/5.2-使用结构体的代码例子.md) | [5.2-rectangles-demo](./05-structs/5.2-rectangles-demo/) |
| 5.3 方法语法 | [5.3](./05-structs/5.3-方法语法.md) | [5.3-method-syntax-demo](./05-structs/5.3-method-syntax-demo/) |
| 5.4 结构体与持久化 | [5.4](./05-structs/5.4-结构体与持久化.md) | [5.4-persist-json-demo](./05-structs/5.4-persist-json-demo/) |

---

## 第 6 章 — 枚举和模式匹配

| 节 | 笔记 | demo |
|----|------|------|
| 6.1 定义枚举 | [6.1](./06-enums-pattern-matching/6.1-定义枚举.md) | [6.1-enums-demo](./06-enums-pattern-matching/6.1-enums-demo/) |
| 6.1.2 | [枚举类型 vs 变体标签](./06-enums-pattern-matching/6.1.2-枚举类型与变体标签.md) | ↑ |
| 6.1.3 | [取出枚举内部数据](./06-enums-pattern-matching/6.1.3-取出枚举内部数据.md) | ↑ |
| 6.2 match | [6.2](./06-enums-pattern-matching/6.2-match.md) | [6.2-match-demo](./06-enums-pattern-matching/6.2-match-demo/) |
| 6.3 if let | [6.3](./06-enums-pattern-matching/6.3-if-let.md) | [6.3-if-let-demo](./06-enums-pattern-matching/6.3-if-let-demo/) |

---

## 第 7 章 — 包、Crate 和模块

| 节 | 笔记 | demo |
|----|------|------|
| 7.1 包和 crate | [7.1 主笔记（hub）](./07-packages-modules/7.1-包和crate.md) | [7.1-packages-crates-demo](./07-packages-modules/7.1-packages-crates-demo/) |
| 7.1.1 | [二进制与库 crate](./07-packages-modules/7.1.1-二进制与库crate.md) | ↑ |
| 7.1.2 | [main 调用 `a.rs`](./07-packages-modules/7.1.2-main调用分文件模块.md) | ↑ |
| 7.1.3 | [Actix-web 项目目录范例](./07-packages-modules/7.1.3-Actix-web项目目录范例.md) | ↑ |
| 7.1.4 | [`src/bin/` 多 exe · HFT workspace](./07-packages-modules/7.1.4-src-bin与多exe.md) | ↑ |
| 7.1.5 | [`lib.rs` 是 Crate 出口（非 Package）](./07-packages-modules/7.1.5-lib.rs是Crate出口.md) | ↑ |
| 7.1.6 | [一个 Package 只能一个 `lib.rs`](./07-packages-modules/7.1.6-一个Package一个lib.rs.md) | ↑ |
| 7.1.7 | [同 Package：lib + main 两 crate](./07-packages-modules/7.1.7-同Package的lib与main.md) | ↑ |
| 7.1.8 | [Package vs Module](./07-packages-modules/7.1.8-Package与Module.md) | ↑ |
| 7.2 模块与私有性 | [7.2](./07-packages-modules/7.2-定义模块来控制作用域与私有性.md) | [7.2-modules-privacy-demo](./07-packages-modules/7.2-modules-privacy-demo/) |
| 7.2.1 | [Crate Root 与模块树](./07-packages-modules/7.2.1-crate-root与模块树.md) | ↑ |
| 7.2.2 | [结构体与字段私有性](./07-packages-modules/7.2.2-结构体与字段私有性.md) | ↑ |
| 7.3 路径 | [7.3](./07-packages-modules/7.3-路径用于引用模块树中的项.md) | [7.3-paths-demo](./07-packages-modules/7.3-paths-demo/) |
| 7.3.1 | [跨 Package 路径 · Workspace 依赖](./07-packages-modules/7.3.1-跨Package路径与Workspace依赖.md) | [7.3-cross-package-paths-demo](./07-packages-modules/7.3-cross-package-paths-demo/) |
| 7.4 use | [7.4](./07-packages-modules/7.4-使用use关键字将名称引入作用域.md) | [7.4-use-demo](./07-packages-modules/7.4-use-demo/) |
| 7.4.1 | [use 作用域 · 无全局 use](./07-packages-modules/7.4.1-use作用域仅限当前文件.md) | [7.5-split-modules-demo](./07-packages-modules/7.5-split-modules-demo/) |
| 7.5 分文件 | [7.5](./07-packages-modules/7.5-将模块分割进不同文件.md) | [7.5-split-modules-demo](./07-packages-modules/7.5-split-modules-demo/) |
| 7.5.1 | [`mod.rs` 里 mod 与 use](./07-packages-modules/7.5.1-mod.rs里的mod与use.md) | ↑ |
| 7.5.2 | [`pub mod` 登记 vs `pub use` 收拢](./07-packages-modules/7.5.2-pub-mod登记与pub-use收拢.md) | ↑ |

---

## 第 8 章 — 常见集合

| 节 | 笔记 | demo |
|----|------|------|
| 8.1 Vector | [8.1-vector.md](./08-collections/8.1-vector.md) | [8.1-vector-demo](./08-collections/8.1-vector-demo/) |
| 8.2 字符串 | [8.2](./08-collections/8.2-string.md) | [8.2-string-demo](./08-collections/8.2-string-demo/) |
| 8.2.1 | [`str`、DST 与定长规则](./08-collections/8.2.1-str与DST定长规则.md) | ↑ |
| 8.2.2 | [不定长数据放哪](./08-collections/8.2.2-不定长数据放哪.md) | ↑ |
| 8.2.3 | [`str` 与 `String` 互转](./08-collections/8.2.3-str与String互转.md) | ↑ |
| 8.2.4 | [UTF-8 字符串实操](./08-collections/8.2.4-UTF-8字符串实操.md) | ↑ |
| 8.2.5 | [四种类型底层结构](./08-collections/8.2.5-四种类型底层结构.md) | ↑ |
| 8.3 HashMap | [8.3-hashmap.md](./08-collections/8.3-hashmap.md) | [8.3-hashmap-demo](./08-collections/8.3-hashmap-demo/) |
| 8.3.1 | [zip → collect → HashMap](./08-collections/8.3.1-zip-collect与HashMap.md) | ↑ |
| 8.3.2 | [创建与删除](./08-collections/8.3.2-创建与删除.md) | ↑ |
| 8.3.3 | [insert · get · 遍历 · Entry](./08-collections/8.3.3-insert-get-遍历与Entry.md) | ↑ |
| 8.3.4 | [Hasher 与 BuildHasher](./08-collections/8.3.4-Hasher与BuildHasher.md) | [8.3.4-hasher-demo](./08-collections/8.3.4-hasher-demo/) |

---

## 第 9 章 — 错误处理

| 节 | 笔记 | demo |
|----|------|------|
| 9.1 panic! | [9.1](./09-error-handling/9.1-panic-与不可恢复的错误.md) | [9.1-panic-demo](./09-error-handling/9.1-panic-demo/) |
| 9.1.1 | [Backtrace 调试](./09-error-handling/9.1.1-Backtrace调试.md) | [9.1.1-backtrace-demo](./09-error-handling/9.1.1-backtrace-demo/) |
| 9.1.2 | [dev/release 与 HFT 编译配置](./09-error-handling/9.1.2-dev-release与HFT编译配置.md) | ↑ |
| 9.1.3 | [catch_unwind 与 panic 模式](./09-error-handling/9.1.3-catch_unwind与panic模式.md) | [9.1.3-catch-unwind-demo](./09-error-handling/9.1.3-catch-unwind-demo/) |
| 9.1.4 | [整数溢出与 wrapping](./09-error-handling/9.1.4-整数溢出与wrapping.md) | [9.1.4-overflow-demo](./09-error-handling/9.1.4-overflow-demo/) |
| 9.2 Result | [9.2](./09-error-handling/9.2-Result-与可恢复的错误.md) | [9.2-result-demo](./09-error-handling/9.2-result-demo/) |
| 9.3 策略 | [9.3](./09-error-handling/9.3-使用panic还是不用panic.md) | [9.3-panic-or-result-demo](./09-error-handling/9.3-panic-or-result-demo/) |

---

## 第 10 章 — 泛型、Trait 与生命周期

| 节 | 笔记 | demo |
|----|------|------|
| 10.1 泛型 | [10.1](./10-generics-traits-lifetimes/10.1-泛型数据类型.md) | [10.1-generics-demo](./10-generics-traits-lifetimes/10.1-generics-demo/) |
| 10.2 Trait | [10.2-trait.md](./10-generics-traits-lifetimes/10.2-trait.md) | [10.2-traits-demo](./10-generics-traits-lifetimes/10.2-traits-demo/) |

**10.2 子笔记**（编号 = 阅读顺序 → [hub 路线图](./10-generics-traits-lifetimes/10.2-trait.md#子笔记阅读顺序1021--1028)）

| 序 | 笔记 | demo |
|----|------|------|
| 10.2.1 | [Trait 约束汇总表](./10-generics-traits-lifetimes/10.2.1-Trait语法速查.md) | ↑ |
| 10.2.2 | [Trait 七种写法详解](./10-generics-traits-lifetimes/10.2.2-Trait七种写法详解.md) | [10.2.2-trait-syntax-demo](./10-generics-traits-lifetimes/10.2.2-trait-syntax-demo/) |
| 10.2.3 | [impl Trait 全解](./10-generics-traits-lifetimes/10.2.3-impl-Trait全解.md) | ↑ |
| 10.2.4 | [type 关键字与关联类型](./10-generics-traits-lifetimes/10.2.4-type关键字与关联类型.md) | [10.2.4-type-keyword-demo](./10-generics-traits-lifetimes/10.2.4-type-keyword-demo/) |
| 10.2.5 | [impl vs dyn Trait 对比](./10-generics-traits-lifetimes/10.2.5-impl与dyn-Trait对比.md) | [10.2.5-impl-dyn-demo](./10-generics-traits-lifetimes/10.2.5-impl-dyn-demo/) |
| 10.2.6 | [vtable 通俗全解](./10-generics-traits-lifetimes/10.2.6-vtable通俗全解.md) | ↑（`-- vtable_story`） |
| 10.2.7 | [dyn 胖指针与虚表](./10-generics-traits-lifetimes/10.2.7-dyn胖指针与虚表.md) | ↑（`-- vtable`） |
| 10.2.8 | [多 Trait 多虚表](./10-generics-traits-lifetimes/10.2.8-多Trait多虚表.md) | ↑（`-- multi_vtable`） |
| 10.3 生命周期（索引） | [10.3](./10-generics-traits-lifetimes/10.3-生命周期与引用有效性.md) | [10.3-lifetimes-demo](./10-generics-traits-lifetimes/10.3-lifetimes-demo/) |

**10.3 专题拆分**

| 子节 | 笔记 |
|------|------|
| 10.3.1 | [悬垂引用](./10-generics-traits-lifetimes/10.3.1-悬垂引用.md) |
| 10.3.2 | [同 `'a` 与红线](./10-generics-traits-lifetimes/10.3.2-同a约束与红线.md) |
| 10.3.3 | [生命周期基础](./10-generics-traits-lifetimes/10.3.3-生命周期基础.md) |
| 10.3.4 | [longest / get_first](./10-generics-traits-lifetimes/10.3.4-longest与get_first.md) |
| 10.3.5 | [显式与隐式生命周期](./10-generics-traits-lifetimes/10.3.5-显式与隐式生命周期.md) |
| 10.3.6 | [结构体 / static / 泛型](./10-generics-traits-lifetimes/10.3.6-结构体-static与泛型.md) |
| 10.3.7 | [匿名生命周期 `'_`](./10-generics-traits-lifetimes/10.3.7-匿名生命周期与下划线.md) |

---

## 第 11 章 — 编写自动化测试

| 节 | 笔记 | demo |
|----|------|------|
| 11.1 编写测试 | [11.1](./11-testing/11.1-如何编写测试.md) | [11.1-tests-demo](./11-testing/11.1-tests-demo/) |
| 11.1.1 | [cfg(test) 与模拟输入](./11-testing/11.1.1-cfg-test与模拟输入.md) | [11.1.1-mock-input-demo](./11-testing/11.1.1-mock-input-demo/) |
| 11.2 控制测试运行 | [11.2](./11-testing/11.2-控制测试如何运行.md) | [11.2-control-tests-demo](./11-testing/11.2-control-tests-demo/) |
| 11.3 测试组织 | [11.3](./11-testing/11.3-测试的组织结构.md) | [11.3-test-organization-demo](./11-testing/11.3-test-organization-demo/) |

---

## 第 12 章 — 构建命令行程序（grep）

| 节 | 笔记 | demo |
|----|------|------|
| 12.1 命令行参数 | [12.1](./12-cli-project/12.1-接受命令行参数.md) | [12.1-minigrep-args-demo](./12-cli-project/12.1-minigrep-args-demo/) |
| 12.2 读取文件 | [12.2](./12-cli-project/12.2-读取文件.md) | [12.2-minigrep-readfile-demo](./12-cli-project/12.2-minigrep-readfile-demo/) |
| 12.3 重构 | [12.3](./12-cli-project/12.3-重构改进模块性和错误处理.md) | [12.3-minigrep-refactor-demo](./12-cli-project/12.3-minigrep-refactor-demo/) |
| 12.4 TDD | [12.4](./12-cli-project/12.4-采用测试驱动开发完善库的功能.md) | [12.4-minigrep-tdd-demo](./12-cli-project/12.4-minigrep-tdd-demo/) · [search](./12-cli-project/12.4-minigrep-search-demo/) |
| 12.5 环境变量 | [12.5](./12-cli-project/12.5-处理环境变量.md) | [12.5-minigrep-env-demo](./12-cli-project/12.5-minigrep-env-demo/) |
| 12.6 stderr | [12.6](./12-cli-project/12.6-将错误信息输出到标准错误而不是标准输出.md) | [12.6-minigrep-stderr-demo](./12-cli-project/12.6-minigrep-stderr-demo/) |

---

## 第 13 章 — 迭代器与闭包

| 节 | 笔记 | demo |
|----|------|------|
| 13.1 闭包 | [13.1](./13-iterators-closures/13.1-闭包.md) | [13.1-closures-demo](./13-iterators-closures/13.1-closures-demo/) |
| 13.1.1 | [闭包四段式语法](./13-iterators-closures/13.1.1-闭包四段式语法.md) | ↑ |
| 13.1.2 | [参数 vs 捕获](./13-iterators-closures/13.1.2-参数与捕获.md) | ↑ |
| 13.1.3 | [类型推导易错点](./13-iterators-closures/13.1.3-闭包类型推导易错点.md) | ↑ |
| 13.1.4 | [Fn / FnMut / FnOnce](./13-iterators-closures/13.1.4-Fn-FnMut-FnOnce三大捕获trait.md) | ↑ |
| 13.1.5 | [move 闭包详解](./13-iterators-closures/13.1.5-move闭包详解.md) | ↑ |
| 13.2 迭代器 | [13.2](./13-iterators-closures/13.2-使用迭代器处理元素序列.md) | [13.2-iterators-demo](./13-iterators-closures/13.2-iterators-demo/) |
| 13.2.1 | [迭代器精讲](./13-iterators-closures/13.2.1-迭代器精讲.md) | ↑ |
| 13.2.2 | [适配器与消费器详解](./13-iterators-closures/13.2.2-适配器与消费器详解.md) | ↑ |
| 13.2.3 | [三者关系与调用链路](./13-iterators-closures/13.2.3-三者关系与调用链路.md) | ↑ |
| 13.2.4 | [惰性演示（适配器/消费器）](./13-iterators-closures/13.2.4-惰性演示-适配器与消费器.md) | ↑ |
| 13.2.5 | [三种 iter 生成方式](./13-iterators-closures/13.2.5-三种迭代生成方式.md) | ↑ |
| 13.2.6 | [迭代器结构体与生命周期](./13-iterators-closures/13.2.6-迭代器结构体与生命周期.md) | ↑ |
| 13.3 改进 I/O 项目 | [13.3](./13-iterators-closures/13.3-改进-IO-项目.md) | [13.3-minigrep-iterators-demo](./13-iterators-closures/13.3-minigrep-iterators-demo/) |
| 13.4 性能对比 | [13.4](./13-iterators-closures/13.4-性能对比-循环-vs-迭代器.md) | [13.4-iterators-performance-demo](./13-iterators-closures/13.4-iterators-performance-demo/) |

---

## 第 14 章 — Cargo 与 Crates.io

| 节 | 笔记 | demo |
|----|------|------|
| 14.1 发布配置 | [14.1](./14-cargo-crates/14.1-采用发布配置自定义构建.md) | [14.1-profiles-demo](./14-cargo-crates/14.1-profiles-demo/) |
| 14.2 发布 crate | [14.2](./14-cargo-crates/14.2-将crate发布到Crates.io.md) | [14.2-crates-io-publish-demo](./14-cargo-crates/14.2-crates-io-publish-demo/) |
| 14.3 工作空间 | [14.3](./14-cargo-crates/14.3-Cargo工作空间.md) | [14.3-workspace-demo](./14-cargo-crates/14.3-workspace-demo/) · [14.3-hft-workspace-demo](./14-cargo-crates/14.3-hft-workspace-demo/) |
| 14.3.1 | [两种打包形态（1 exe vs N exe）](./14-cargo-crates/14.3.1-Workspace与微服务.md) | [14.3-packaging-demo](./14-cargo-crates/14.3-packaging-demo/) · [14.3-hft-workspace-demo](./14-cargo-crates/14.3-hft-workspace-demo/) |
| 14.4 cargo install | [14.4](./14-cargo-crates/14.4-使用cargo-install从Crates.io安装二进制文件.md) | — |
| 14.5 自定义命令 | [14.5](./14-cargo-crates/14.5-Cargo自定义扩展命令.md) | — |

---

## 第 15 章 — 智能指针

| 节 | 笔记 | demo |
|----|------|------|
| 15.1 Box | [15.1](./15-smart-pointers/15.1-使用Box指向堆上的数据.md) · [15.1.1 `&**self`](./15-smart-pointers/15.1.1-Box的Deref与双层星号.md) | [15.1-box-demo](./15-smart-pointers/15.1-box-demo/) |
| 15.2 Deref | [15.2](./15-smart-pointers/15.2-通过Deref将智能指针当作引用.md) · [15.2.1 嵌套/坑点](./15-smart-pointers/15.2.1-Deref嵌套可变与编译坑.md) | [15.2-deref-demo](./15-smart-pointers/15.2-deref-demo/) |
| 15.3 Drop | [15.3](./15-smart-pointers/15.3-使用Drop运行清理代码.md) · [15.3.1](./15-smart-pointers/15.3.1-Drop顺序与进阶场景.md) · [15.3.2 Socket RAII](./15-smart-pointers/15.3.2-Drop与网络Socket-RAII.md) | [15.3-drop-demo](./15-smart-pointers/15.3-drop-demo/) |
| 15.4 Rc | [15.4](./15-smart-pointers/15.4-Rc引用计数智能指针.md) · [15.4.1 限制/选型](./15-smart-pointers/15.4.1-Rc限制对比与循环引用.md) | [15.4-rc-demo](./15-smart-pointers/15.4-rc-demo/) |
| 15.5 RefCell | [15.5](./15-smart-pointers/15.5-RefCell与内部可变性.md)（Rc+Cell/RefCell） | [15.5-refcell-demo](./15-smart-pointers/15.5-refcell-demo/) |
| 15.6 循环引用 | [15.6](./15-smart-pointers/15.6-引用循环与Weak.md)（Weak · 节点环） | [15.6-reference-cycle-demo](./15-smart-pointers/15.6-reference-cycle-demo/) |

---

## 第 16 章 — 无畏并发

| 节 | 笔记 | demo |
|----|------|------|
| 16.1 线程 | [16.1](./16-fearless-concurrency/16.1-使用线程同时运行代码.md) | [16.1-threads-demo](./16-fearless-concurrency/16.1-threads-demo/) |
| 16.2 消息传递 | [16.2](./16-fearless-concurrency/16.2-消息传递与通道.md) | [16.2-mpsc-demo](./16-fearless-concurrency/16.2-mpsc-demo/) |
| 16.3 共享状态 | [16.3](./16-fearless-concurrency/16.3-共享状态并发.md) | [16.3-mutex-arc-demo](./16-fearless-concurrency/16.3-mutex-arc-demo/) |
| 16.4 Send / Sync | [16.4](./16-fearless-concurrency/16.4-Send与Sync.md) | [16.4-send-sync-demo](./16-fearless-concurrency/16.4-send-sync-demo/) |

---

## 第 17 章 — 面向对象编程特性

| 节 | 笔记 | demo |
|----|------|------|
| 17.1 OOP 特征 | [17.1](./17-oop/17.1-面向对象语言的特征.md)（对象 · 封装 · Trait 替代继承 · 多态） | [17.1-averaged-collection-demo](./17-oop/17.1-averaged-collection-demo/) |
| 17.2 Trait 对象 | [17.2](./17-oop/17.2-为使用不同类型的值而设计的trait对象.md)（DST · 对象安全 · 易错点 · GUI demo） | [17.2-trait-objects-demo](./17-oop/17.2-trait-objects-demo/) |
| 17.3 状态模式 | [17.3](./17-oop/17.3-状态模式与博客工作流.md) | [17.3-blog-workflow-demo](./17-oop/17.3-blog-workflow-demo/) |

---

## 第 18 章 — 模式

| 节 | 笔记 | demo |
|----|------|------|
| 18.1 模式位置 | [18.1](./18-patterns/18.1-所有可能会用到模式的位置.md) | [18.1-pattern-locations-demo](./18-patterns/18.1-pattern-locations-demo/) |
| 18.2 可反驳性 | [18.2](./18-patterns/18.2-可反驳性-模式是否会匹配失效.md)（边界 · 易错 · 整合 demo） | [18.2-refutability-demo](./18-patterns/18.2-refutability-demo/) |
| 18.3 模式语法 | [18.3](./18-patterns/18.3-模式语法.md)（解构 · `_`/`..` · 守卫 · `@` · 易错点） | [18.3-pattern-syntax-demo](./18-patterns/18.3-pattern-syntax-demo/) |

---

## 第 19 章 — 高级特性

→ **章节导读**：[19-章节导读](./19-advanced-features/19-章节导读.md)（结构 · 考点速览 · 一键 Demo）

| 节 | 笔记 | demo |
|----|------|------|
| 19.1 不安全 Rust | [19.1](./19-advanced-features/19.1-不安全Rust.md)（五大超能力 · FFI · 封装） | [19.1-unsafe-rust-demo](./19-advanced-features/19.1-unsafe-rust-demo/) |
| 19.2 高级 trait | [19.2](./19-advanced-features/19.2-高级trait.md)（关联类型 · FQS · supertrait · newtype · 完整 demo） | [19.2-advanced-traits-demo](./19-advanced-features/19.2-advanced-traits-demo/) |
| 19.3 高级类型 | [19.3](./19-advanced-features/19.3-高级类型.md)（newtype · `!` · DST · `?Sized` · 完整 demo） | [19.3-advanced-types-demo](./19-advanced-features/19.3-advanced-types-demo/) |
| 19.4 高级函数与闭包 | [19.4](./19-advanced-features/19.4-高级函数与闭包.md)（`fn` · `F:Fn` · 返回 `Box<dyn Fn>` · 完整 demo） | [19.4-advanced-functions-closures-demo](./19-advanced-features/19.4-advanced-functions-closures-demo/) |
| 19.5 宏 | [19.5](./19-advanced-features/19.5-宏.md)（macro_rules! · 过程宏 workspace · cargo expand） | [19.5-macros-demo](./19-advanced-features/19.5-macros-demo/) |

---

## 学习进度（粗分）

| 段落 | 章 | 建议 |
|------|-----|------|
| 基础 | 1–5 | 语法、所有权 |
| 核心 | 6–10 | 枚举、模块、集合、错误、泛型与生命周期 |
| 项目与工具 | 11–14 | 测试、grep 项目、迭代器、Cargo |
| 进阶 | 15–19 | 智能指针、并发、OOP、模式、unsafe/宏 |

详细勾选见 [`Rust学习笔记.md`](../Rust学习笔记.md)。

---

## 附录

索引：[appendix/README.md](./appendix/README.md)

| 附录 | 笔记 | 说明 |
|------|------|------|
| A | [A.1 关键字](./appendix/A.1-关键字.md) | 保留字、`r#` 原始标识符 |
| B | [B.2 运算符与符号](./appendix/B.2-运算符与符号.md) | 运算符与语法符号速查 |
| C | [C.3 可 derive 的 Trait](./appendix/C.3-可derive的Trait.md) | `#[derive(...)]` 标准库列表 |
| D | [D.4 实用开发工具](./appendix/D.4-实用开发工具.md) | fmt / clippy / rust-analyzer |
| E | [E.5 Editions](./appendix/E.5-Editions.md) | Edition 与 Channel 区别 |
| F | [F.6 译本](./appendix/F.6-译本.md) | 社区翻译 |
| G | [G.7 Nightly Rust](./appendix/G.7-Nightly-Rust与发布渠道.md) | Stable/Beta/Nightly；链 [README § 工具链](../README.md#rust-工具链stablenightly--edition) |
