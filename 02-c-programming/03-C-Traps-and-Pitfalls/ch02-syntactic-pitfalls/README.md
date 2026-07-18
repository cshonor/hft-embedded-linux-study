# 第 2 章 语法「陷阱」

**Syntactic Pitfalls** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

词法分析（[ch01](../ch01-lexical-pitfalls/)）切完 token 后，编译器按 **语法规则** 组装表达式与语句。本章陷阱多为 **token 组合逻辑错误**——**常不报错，逻辑却完全跑偏**。

> **人脑缩进逻辑 ≠ 编译器语法匹配逻辑**

## 小节索引

| 节 | 主题 | 核心坑 |
|----|------|--------|
| [2.1](./2.1-运算符优先级.md) | 优先级 | `x & mask == 2` 分组错误 |
| [2.2](./2.2-else就近匹配.md) | 悬垂 else | else 绑最近 if |
| [2.3](./2.3-函数声明与调用.md) | 调用语法 | `handler` vs `handler()` |
| [2.4](./2.4-括号缺失与语句体.md) | 缺 `{}` | 仅下一语句受控 |
| [2.5](./2.5-逗号运算符与分隔符.md) | 逗号 | 运算符 vs 实参分隔 |
| [2.6](./2.6-数组与结构体语法.md) | `[]` `.` | `i[a]`、`&st.x` |

## 工程规范（内核 / HFT / 嵌入式）

1. **位运算 + 比较**：一律加括号 `(x & m) == v`
2. **所有分支/循环**：强制 `{}`
3. **函数**：头文件原型，禁隐式 int（`-Werror=implicit-function-declaration`）
4. **复杂表达式**：不用逗号运算符拼逻辑

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch01 词法](../ch01-lexical-pitfalls/) |
| **后置** | [ch03 语义](../ch03-semantic-pitfalls/) — 类型、指针、UB |
| **交叉** | [Expert C ch08 优先级表](../04-Expert-C-Programming/ch08-halloween-vs-christmas/operator-precedence-cheatsheet.md) |

## Demo

```bash
cd demo
make all
./demo01_bitwise/main
./demo02_dangling_else/main
./demo03_func_ptr/main
./demo04_missing_braces/main
./demo05_comma/main
```

## 面试题

1. `if (x & 0x02 == 2)` 实际判断什么？如何写对？
2. 悬垂 else 绑定规则？如何修复？
3. `signal(sig, handler())` 错在哪？
4. `if (f) a=1; b=2;` 执行语义？
5. 逗号运算符与函数实参逗号有何区别？
