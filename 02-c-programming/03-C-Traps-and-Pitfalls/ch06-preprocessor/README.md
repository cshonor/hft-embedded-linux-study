# 第 6 章 预处理器

**Preprocessor** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

[ch05 库函数](../ch05-library-functions/) 之后，本章聚焦 **编译前文本替换**：宏展开、条件编译、头文件防护 —— 预处理器 **不理解 C 类型与优先级**，是内核/底层宏 bug 最高发区。

```text
  .c ──► 预处理器 ──► 翻译单元 ──► 编译 ──► .o
              ↑
         纯文本 #define / #include / #if
```

## 小节索引

| 节 | 主题 |
|----|------|
| [6.1](./6.1-宏参数括号.md) | 参数/整体括号、优先级 |
| [6.2](./6.2-宏副作用重复.md) | `i++` 重复求值 |
| [6.3](./6.3-多行宏与分号.md) | if-else、`do-while(0)` |
| [6.4](./6.4-字符串化与连接.md) | `#`、`##` |
| [6.5](./6.5-宏名冲突.md) | 宏覆盖 typedef/关键字 |
| [6.6](./6.6-头文件防护.md) | `#ifndef` / `#pragma once` |
| [6.7](./6.7-条件编译与注释.md) | 注释无法屏蔽 `#ifdef` |
| [6.8](./6.8-宏无类型检查.md) | `MAX` 混类型 |
| [6.9](./6.9-空宏与defined.md) | `#ifdef` vs `#if DEBUG` |

## 底层宏编写规范

1. 参数与整体 **双层括号**
2. **禁止** 副作用实参传入宏
3. 多语句宏用 **`do { ... } while (0)`**
4. 常量宏 **全大写**；逻辑用 **inline 函数**
5. 头文件 **include guard**
6. 大段禁用代码用 **`#if 0`**，不靠块注释包 `#ifdef`

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch05 库函数](../ch05-library-functions/) |
| **后置** | [ch07 可移植性](../ch07-portability-pitfalls/) |
| **交叉** | [ch04 4.8 头文件保护](../ch04-linking/4.8-头文件保护.md) |

## Demo

```bash
cd demo && make all
./demo01_parens/main
./demo02_side_effect/main
./demo03_dowhile/main
./demo04_stringify/main
./demo05_token_paste/main
make -C demo06_cond_compile debug=1
./demo07_max/main
```

## 面试题

1. `#define SQUARE(x) x*x` 与 `((x)*(x))` 区别？
2. 为何 `SQUARE(i++)` 危险？inline 如何解决？
3. `do-while(0)` 宏解决什么问题？
4. `#` 与 `##` 各做什么？
5. `#ifdef DEBUG` 与 `#if DEBUG` 区别？
