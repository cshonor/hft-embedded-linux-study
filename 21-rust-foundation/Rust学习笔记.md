# Rust 学习笔记

> 基于《Rust程序设计语言》教程大纲整理  
> **全书导航**（笔记 + demo 链接）：[00-Book/Book-本书目录.md](00-Book/Book-本书目录.md)

---

## 1. 入门指南

### 1.1 安装
- [ ] Rustup 安装与配置
- [ ] rustc、cargo 工具链

### 1.2 Hello, World!
- [ ] 编写第一个 Rust 程序
- [ ] 编译与运行

### 1.3 Hello, Cargo!
- [ ] Cargo 项目结构
- [ ] 构建、运行、检查命令

---

## 2. 猜数字游戏

- [ ] 项目实战：实现猜数字游戏
- [ ] 综合运用输入输出、随机数、循环等

---

## 3. 通用编程概念

### 3.1 变量和可变性
- [ ] `let` 与 `let mut`
- [ ] 常量 `const`
- [ ] 遮蔽（shadowing）

### 3.2 数据类型
- [ ] 标量类型：整数、浮点、布尔、字符
- [ ] 复合类型：元组、数组

### 3.3 函数
- [ ] 函数定义与调用
- [ ] 参数与返回值
- [ ] 表达式与语句

### 3.4 注释
- [ ] `//` 行注释
- [ ] `///` 文档注释

### 3.5 控制流
- [ ] `if` 表达式
- [ ] `loop`、`while`、`for` 循环

---

## 4. 认识所有权

### 4.1 什么是所有权？
- [ ] 所有权规则
- [ ] 移动（move）与克隆（clone）
- [ ] 栈与堆

### 4.2 引用与借用
- [ ] 不可变引用 `&`
- [ ] 可变引用 `&mut`
- [ ] 借用规则

### 4.3 切片 slice
- [ ] 字符串切片 `&str`
- [ ] 数组切片 `&[T]`

---

## 5. 使用结构体组织关联数据

### 5.1 定义和举例说明结构体
- [ ] 结构体定义
- [ ] 实例化与字段访问

### 5.2 使用结构体的代码例子
- [ ] 结构体在实战中的应用

### 5.3 方法语法
- [ ] `impl` 块
- [ ] 方法 `&self` 与关联函数

---

## 6. 枚举和模式匹配

### 6.1 定义枚举
- [ ] 枚举定义与变体
- [ ] `Option<T>` 与 `Result<T, E>`

### 6.2 match 控制流运算符
- [ ] 匹配分支
- [ ] 穷尽性检查
- [ ] 通配模式

### 6.3 if let 简单控制流
- [ ] 简化单分支匹配

---

## 7. 使用包、Crate 和模块管理不断增长的项目

### 7.1 包和 crate
- [ ] 包（package）与 crate 概念
- [ ] 库 crate 与二进制 crate

### 7.2 定义模块来控制作用域与私有性
- [ ] `mod` 定义模块
- [ ] `pub` 公开性

### 7.3 路径用于引用模块树中的项
- [ ] 绝对路径 `crate::`
- [ ] 相对路径 `self::`、`super::`

### 7.4 使用 use 关键字将名称引入作用域
- [ ] `use` 引入
- [ ] 重导出 `pub use`

### 7.5 将模块分割进不同文件
- [ ] 模块文件组织

---

## 8. 常见集合

### 8.1 使用 vector 存储一列值
- [ ] `Vec<T>` 创建、增删改查
- [ ] 遍历

### 8.2 使用字符串存储 UTF-8 编码的文本
- [ ] `String` 与 `&str`
- [ ] 字符串操作与索引

### 8.3 在哈希 map 中存储键和关联值
- [ ] `HashMap<K, V>`
- [ ] 插入、查找、更新

---

## 9. 错误处理

### 9.1 panic! 与不可恢复的错误
- [ ] `panic!` 宏
- [ ] unwinding 与 abort

### 9.2 Result 与可恢复的错误
- [ ] `Result<T, E>` 枚举
- [ ] `?` 运算符
- [ ] `unwrap`、`expect`

### 9.3 panic! 还是不 panic!
- [ ] 错误处理策略选择

---

## 10. 泛型、trait 与生命周期

### 10.1 泛型数据类型
- [ ] 函数泛型
- [ ] 结构体、枚举泛型
- [ ] 方法中的泛型

### 10.2 trait: 定义共享的行为
- [ ] 定义与实现 trait
- [ ] trait 作为参数
- [ ] trait bound 语法
- [ ] 常见 trait：`Debug`、`Clone`、`Copy` 等

### 10.3 生命周期与引用有效性
- [ ] [10.3 索引](00-Book/10-generics-traits-lifetimes/10.3-生命周期与引用有效性.md)
- [ ] [10.3.1 悬垂引用](00-Book/10-generics-traits-lifetimes/10.3.1-悬垂引用.md)
- [ ] [10.3.2 同 `'a` 与红线](00-Book/10-generics-traits-lifetimes/10.3.2-同a约束与红线.md)
- [ ] [10.3.3 生命周期基础](00-Book/10-generics-traits-lifetimes/10.3.3-生命周期基础.md)
- [ ] [10.3.4 longest / get_first](00-Book/10-generics-traits-lifetimes/10.3.4-longest与get_first.md)
- [ ] [10.3.5 显式与隐式](00-Book/10-generics-traits-lifetimes/10.3.5-显式与隐式生命周期.md)
- [ ] [10.3.6 结构体 / static / 泛型](00-Book/10-generics-traits-lifetimes/10.3.6-结构体-static与泛型.md)

---

## 11. 编写自动化测试

### 11.1 如何编写测试
- [ ] `#[test]` 属性
- [ ] `assert!`、`assert_eq!`、`assert_ne!`

### 11.2 控制测试如何运行
- [ ] `cargo test` 选项
- [ ] 并行与串行

### 11.3 测试的组织结构
- [ ] 单元测试
- [ ] 集成测试
- [ ] `tests/` 目录

---

## 12. 一个 I/O 项目：构建命令行程序

### 12.1 接受命令行参数
- [ ] `std::env::args`

### 12.2 读取文件
- [ ] `std::fs` 文件操作

### 12.3 重构以改进模块化与错误处理
- [ ] 项目结构优化

### 12.4 采用测试驱动开发完善库的功能
- [ ] TDD 实践

### 12.5 处理环境变量
- [ ] `std::env`

### 12.6 将错误信息输出到标准错误而不是标准输出
- [ ] `stderr` 与 `stdout`
- [ ] `eprintln!` 宏

---

## 13. Rust 中的函数式语言功能：迭代器与闭包

### 13.1 闭包：可以捕获其环境的匿名函数
- [ ] 闭包语法
- [ ] 捕获方式：借入、可变借入、获取所有权
- [ ] `Fn`、`FnMut`、`FnOnce` trait

### 13.2 使用迭代器处理元素序列
- [ ] `Iterator` trait
- [ ] `next`、`map`、`filter`、`collect`
- [ ] 惰性求值

### 13.3 改进之前的 I/O 项目
- [ ] 用迭代器重构

### 13.4 性能比较：循环对迭代器
- [ ] 零成本抽象

---

## 14. 更多关于 Cargo 和 Crates.io 的内容

### 14.1 采用发布配置自定义构建
- [ ] `[profile.release]`
- [ ] `dev` 与 `release` 配置

### 14.2 将 crate 发布到 Crates.io
- [ ] 文档与元数据
- [ ] 版本号语义化

### 14.3 Cargo 工作空间
- [ ] 多 crate 项目管理
- [ ] `workspace` 配置

### 14.4 使用 cargo install 从 Crates.io 安装二进制文件
- [ ] 全局安装工具

### 14.5 Cargo 自定义扩展命令
- [ ] `cargo-*` 子命令

---

## 15. 智能指针

### 15.1 使用 Box\<T\> 指向堆上的数据
- [x] 堆分配 · Deref · Drop → [15.1](./00-Book/15-smart-pointers/15.1-使用Box指向堆上的数据.md)
- [x] 递归类型 · Cons · `&**self` → [15.1.1](./00-Book/15-smart-pointers/15.1.1-Box的Deref与双层星号.md)

### 15.2 使用 Deref trait 将智能指针当作常规引用处理
- [x] 解引用 `*` → [15.2](./00-Book/15-smart-pointers/15.2-通过Deref将智能指针当作引用.md)
- [x] `Deref` 与 `DerefMut` → [15.2.1 嵌套/坑点](./00-Book/15-smart-pointers/15.2.1-Deref嵌套可变与编译坑.md)

### 15.3 使用 Drop Trait 运行清理代码
- [x] 析构逻辑 → [15.3](./00-Book/15-smart-pointers/15.3-使用Drop运行清理代码.md)
- [x] `drop` 函数 · `mem::drop` → [15.3.1](./00-Book/15-smart-pointers/15.3.1-Drop顺序与进阶场景.md)

### 15.4 Rc\<T\> 引用计数智能指针
- [x] 多所有权 · vs Box → [15.4](./00-Book/15-smart-pointers/15.4-Rc引用计数智能指针.md)
- [x] `strong_count` · 限制/循环引用 → [15.4.1](./00-Book/15-smart-pointers/15.4.1-Rc限制对比与循环引用.md)

### 15.5 RefCell\<T\> 与内部可变性模式
- [x] 运行时借用 · `Rc<Cell>` / `Rc<RefCell>` → [15.5](./00-Book/15-smart-pointers/15.5-RefCell与内部可变性.md)

### 15.6 引用循环会导致内存泄漏
- [x] 循环引用 · `Weak<T>` → [15.6](./00-Book/15-smart-pointers/15.6-引用循环与Weak.md)

---

## 16. 无畏并发

### 16.1 使用线程同一时间运行代码
- [ ] `std::thread::spawn`
- [ ] `join` 等待线程

### 16.2 使用消息传递在线程间通信
- [ ] 信道（channel）
- [ ] `mpsc`（多生产者单消费者）

### 16.3 共享状态并发
- [ ] `Mutex<T>`
- [ ] `Arc<T>` 多线程共享所有权

### 16.4 使用 Sync 与 Send Trait 的可扩展并发
- [ ] `Send`：可跨线程传递所有权
- [ ] `Sync`：可跨线程共享引用

---

## 17. Rust 的面向对象编程特性

### 17.1 面向对象语言的特点
- [x] 对象 · 封装 · 继承替代 · 多态 → [17.1](./00-Book/17-oop/17.1-面向对象语言的特征.md)

### 17.2 为使用不同类型的值而设计的 trait 对象
- [x] dyn Trait · DST · 对象安全 · 易错点 → [17.2](./00-Book/17-oop/17.2-为使用不同类型的值而设计的trait对象.md)

### 17.3 面向对象设计模式的实现
- [ ] 状态模式等

---

## 18. 模式和匹配

### 18.1 所有可能会用到模式的位置
- [x] match / if let / while let / for / let / 参数 → [18.1](./00-Book/18-patterns/18.1-所有可能会用到模式的位置.md)

### 18.2 Refutability（可反驳性）：模式是否会匹配失效
- [x] 可反驳 vs 不可反驳 · 边界表 → [18.2](./00-Book/18-patterns/18.2-可反驳性-模式是否会匹配失效.md)

### 18.3 模式语法
- [x] 字面量 · 解构 · `_`/`..` · 守卫 · `@ → [18.3](./00-Book/18-patterns/18.3-模式语法.md)

---

## 19. 高级特征

→ [19 章导读](./00-Book/19-advanced-features/19-章节导读.md)

### 19.1 不安全的 Rust
- [x] 五大超能力 · 示例 · 易错点 → [19.1](./00-Book/19-advanced-features/19.1-不安全Rust.md)

### 19.2 高级 trait
- [x] 关联类型 · FQS · supertrait · newtype → [19.2](./00-Book/19-advanced-features/19.2-高级trait.md)

### 19.3 高级类型
- [x] `!` · DST · ZST · 别名 vs newtype → [19.3](./00-Book/19-advanced-features/19.3-高级类型.md)

### 19.4 高级函数与闭包
- [x] `fn` · `Fn*` · 返回闭包 → [19.4](./00-Book/19-advanced-features/19.4-高级函数与闭包.md)

### 19.5 宏
- [x] `macro_rules!` · 过程宏 · hygiene → [19.5](./00-Book/19-advanced-features/19.5-宏.md)

---

## 学习进度追踪

| 章节 | 状态 | 备注 |
|------|------|------|
| 1-5  | ⬜   | 基础篇 |
| 6-10 | ⬜   | 核心概念 |
| 11-14| ⬜   | 项目与工具 |
| 15-19| ⬜   | 进阶篇 |

---

*持续更新中...*

