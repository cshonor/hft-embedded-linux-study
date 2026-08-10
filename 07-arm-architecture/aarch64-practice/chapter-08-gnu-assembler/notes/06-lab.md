# 8.6 实验要点

> 来源：§8.6 · 精读 · [章总览](section-0-本章完整概述.md)

## 实验列表

| 实验 | 内容 | 平台 |
|------|------|------|
| 8-1 | GNU as 基本语法练习 | QEMU |
| 8-2 | 伪指令和段的使用 | QEMU |
| 8-3 | 宏定义和使用 | QEMU |
| 8-4 | C 调用汇编函数 | QEMU |
| 8-5 | 汇编调用 C 函数 | QEMU |

## 自测题

1. 如何用命令行把 .s 文件编译为可执行文件？
<details><summary>答案</summary>
```bash
aarch64-linux-gnu-gcc -c test.s -o test.o
aarch64-linux-gnu-gcc test.o -o test
```
</details>

2. `objdump -d` 的输出中如何判断某行是代码还是数据？
<details><summary>答案</summary>
`objdump -d` 只反汇编代码段（.text）。数据段需要 `objdump -s -j .data` 查看。
</details>

3. C 调用汇编函数时，链接器怎么知道函数地址？
<details><summary>答案</summary>
汇编函数用 `.global func_name` 声明为全局符号。链接器在符号表中查找，解析调用地址。
</details>

## 参考与延伸

- 原书 §8.6
- [8.5 C↔汇编互调](05-c-asm-interop.md)
