# 第 4 章 连接

**Linking** — Andrew Koenig, *C Traps and Pitfalls*

## 本章目标

[ch00–ch03](../ch00-introduction/) 侧重 **单文件** 词法/语法/语义；本章进入 **多 `.c` → `.o` → 链接器** 阶段。陷阱多为 **编译通过、链接或运行才暴露**。

```text
  a.c ──► a.o ─┐
  b.c ──► b.o ─┼──► ld ──► a.out
  libfoo.a ────┘      符号解析 + 重定位
```

## 小节索引

| 节 | 主题 |
|----|------|
| [4.1](./4.1-extern声明与定义.md) | `extern` vs 定义、头文件误写 `int g;` |
| [4.2](./4.2-bss与data全局变量.md) | `.bss` / `.data` |
| [4.3](./4.3-static内部链接.md) | `static` 文件内可见 |
| [4.4](./4.4-类型不匹配链接.md) | `arr[]` vs `extern *arr` **静默灾难** |
| [4.5](./4.5-静态库链接顺序.md) | `.a` 左→右扫描 |
| [4.6](./4.6-强弱符号.md) | 多文件 tentative 共享 |
| [4.7](./4.7-未定义引用.md) | 漏 `.o` / 漏库 |
| [4.8](./4.8-头文件保护.md) | include guard |

## 底层开发规范

1. 全局：**头文件 `extern`，单 `.c` 定义**
2. 模块私有：**`static`** 全局与工具函数
3. 静态库：**调用在前，库在后**
4. 头文件：**include guard**，只声明不定义
5. 跨文件符号：**统一头文件类型**
6. 链接后：`nm` / `readelf -s` 核对符号

## 前后章节

| | 章节 |
|---|------|
| **前置** | [ch03 语义](../ch03-semantic-pitfalls/) |
| **后置** | [ch05 库函数](../ch05-library-functions/) |
| **交叉** | [Expert C ch05 链接](../04-Expert-C-Programming/ch05-thinking-of-linking/) |

## Demo

```bash
cd demo && make all
./demo01_extern/main
./demo02_extern_type/use_wrong    # 段错误：类型不匹配
./demo02_extern_type/use_correct
./demo03_static_lib/demo
./demo04_bss_data/main            # nm 显示 B/D 段
make -C demo05_undef link_fail      # undefined reference
```

## 面试题

1. `extern int a;` 与 `int a;` 区别？
2. 为何 `extern char *p` 不能对应 `char p[] = "x"`？
3. `static` 全局与 `extern` 全局链接属性？
4. 静态库为何要注意链接顺序？
5. `undefined reference` 与 `multiple definition` 各什么原因？
