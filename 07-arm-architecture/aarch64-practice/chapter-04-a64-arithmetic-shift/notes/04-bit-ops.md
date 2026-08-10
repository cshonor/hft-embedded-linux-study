# 4.4 位操作指令

> 来源：§4.4 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

AND/ORR/EOR/BIC 等位操作指令及其在掩码/标志位操作中的应用。

## 核心要点

| 指令 | 作用 |
|------|------|
| AND | 按位与 |
| ORR | 按位或 |
| EOR | 按位异或 |
| BIC | 位清除（AND NOT） |
| TST | 测试（≡ AND XZR） |
| MOVN | 取反移动 |

常见用法：
- `AND x0, x0, #0xFF` — 提取低 8 位
- `ORR x0, x0, #(1<<4)` — 置位第 4 位
- `BIC x0, x0, #(1<<4)` — 清除第 4 位
- `EOR x0, x0, #(1<<4)` — 翻转第 4 位
- `TST x0, #(1<<4)` — 测试第 4 位（设 Z 标志）

## HFT 关联

位操作在协议解析和标志管理中至关重要：
- 市场数据协议的位域标志（如 FIX 消息头标志位）用 AND/TST 提取
- 订单状态标志用 ORR/BIC 设置/清除
- EOR 翻转位用于状态切换（如开关中断屏蔽）
- BIC 比 `AND NOT` 更简洁，一条指令完成位清除

## 自测题

1. 如何清除 x0 的第 7 位而不影响其他位？
<details><summary>答案</summary>
`bic x0, x0, #(1 << 7)` 或 `and x0, x0, #0xFFFFFFFFFFFFFF7F`（BIC 更直观简洁）。
</details>

2. `TST x0, #0xF` 执行后如何判断 x0 低 4 位是否全为 0？
<details><summary>答案</summary>
TST 等价于 AND XZR。如果 x0 低 4 位全 0，AND 结果为 0 → Z=1。用 `B.EQ` 判断 Z=1 即可。
</details>

3. EOR 有什么特殊用途？
<details><summary>答案</summary>
1. 翻转特定位（toggle）：`eor x0, x0, #mask`
2. 清零：`eor x0, x0, x0` → x0=0（同 MOV XZR）
3. 简单加密：异或密钥可加密/解密
4. 无临时变量交换：`eor x0,x0,x1; eor x1,x1,x0; eor x0,x0,x1`
</details>

## 参考与延伸

- 原书 §4.4
- [4.5 位段提取](05-bit-field.md)
- ARM ARM §C3.4
