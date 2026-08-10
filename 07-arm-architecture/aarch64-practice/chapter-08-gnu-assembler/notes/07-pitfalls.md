# 8.7 易错点清单

> 来源：§8.7 · 精读 · [章总览](section-0-本章完整概述.md)

## 4 大易错点

1. **标号不顶格** → 被当作指令
2. **.align 参数**在不同架构含义不同（AArch64 = 2^n，x86 = n）
3. **callee-saved 寄存器**忘记保存 → 破坏调用者状态
4. **.global 缺失** → 链接器找不到符号

## 自测题

1. 以下代码为什么链接报错 `undefined reference to my_func`？
```asm
my_func:
    ret
```
<details><summary>答案</summary>
缺少 `.global my_func` 声明。没有 `.global`，符号只在当前文件内可见。链接器找不到定义。
</details>

2. `.align 3` 在 AArch64 和 x86 上分别对齐多少字节？
<details><summary>答案</summary>
AArch64: 2^3 = 8 字节。x86: 3 字节。跨架构代码需用 `.balign`（统一为字节数）。
</details>

3. 汇编函数中使用 X19 但没保存，会有什么后果？
<details><summary>答案</summary>
X19 是 callee-saved。调用者期望函数返回后 X19 不变。修改了不保存恢复 → 调用者数据被破坏 → 难以调试的 bug。
</details>

## 参考与延伸

- 原书 §8.7
- [8.3 段](03-sections.md)
