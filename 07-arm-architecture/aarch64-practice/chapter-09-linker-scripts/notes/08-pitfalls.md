# 9.8 易错点清单

> 来源：§9.8 · 精读 · [章总览](section-0-本章完整概述.md)

## 4 大易错点

1. **VMA/LMA 混淆** → 启动后数据访问错误
2. **位置计数器忘记对齐** → 段不对齐
3. **ENTRY 指定错误** → 程序入口地址不对
4. **段属性遗漏** → 代码段可写或数据段可执行

## 自测题

1. 程序启动后立即 crash，可能是什么链接问题？
<details><summary>答案</summary>
1. VMA≠LMA 但启动代码没拷贝 .data 2. ENTRY 指定的符号不存在 3. .text 没有可执行权限 4. 位置计数器设错
</details>

2. 以下链接脚本有什么问题？
```ld
.text 0x80000 : { *(.text) }
.data 0x80008 : { *(.data) }
```
<details><summary>答案</summary>
.data 地址可能和 .text 重叠，且 0x80008 不是 8 字节对齐。应用位置计数器 + ALIGN。
</details>

3. 为什么不能让 .text 段可写？
<details><summary>答案</summary>
安全风险：可写代码段可被注入 shellcode。现代 OS 把 .text 标记为只读+可执行。
</details>

## 参考与延伸

- 原书 §9.8
- [9.3 VMA vs LMA](03-vma-lma.md)
