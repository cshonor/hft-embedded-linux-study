# 03 · rlox — Rust Lox 解释器

> 索引：[08 实战项目](../README.md) · 原书：[Crafting Interpreters · jlox](../../06_Compilers-and-LLVM-Learning/01_Crafting-Interpreters/part02_jlox/README.md)

## 是什么

[Crafting Interpreters](https://craftinginterpreters.com/) **Lox** 语言的 Rust 实现，路线 **Part II 树遍历（jlox）**；后续可分支 **Part III 字节码 VM（clox）**。

## 架构（当前骨架）

```text
source ──► Scanner ──► Parser ──► Interpreter (TODO) ──► Value
                REPL (stdin)
```

## 运行

```bash
cargo run -p rlox
# 或
cargo run -p rlox -- repl
```

输入 `exit` 退出 REPL。完整 Lox 语法待逐步实现。

## Roadmap

- [x] Token 枚举 · Scanner 骨架 · REPL
- [ ] Parser（递归下降）· AST
- [ ] 表达式求值 · 环境链
- [ ] 函数 · 闭包 · 类 · 继承
- [ ] `tests/lox_suite/` 黄金用例
- [ ] （可选）clox 字节码分支

## 设计取舍

1. **先 jlox 后 clox** — 可控进度、与 06 笔记 Part II 同步。
2. **错误不 panic** — 行号 + 友好消息，REPL 继续。
3. **零/少依赖** — 核心手写，便于读源码学习。

## 对外描述（GitHub）

> Lox interpreter in Rust (Crafting Interpreters / tree-walk jlox track).
