## 3.6 控制

> ↔ [Harris §6.6 异常/中断](../../../00-digital-logic-cpu/ch06_architecture/6.6_其他主题.md)

### 3.6.1 条件码

`cmp`、`test`、算术指令会设置 **EFLAGS** 片段：

| 标志 | 含义 |
|------|------|
| **ZF** | 结果为 0 |
| **SF** | 结果为负 |
| **CF** | 无符号进位/借位 |
| **OF** | 有符号溢出 |

### 3.6.2 访问条件码

- `setcc` — 根据条件把 0/1 写入字节寄存器
- `cmovcc` — **条件传送**，无跳转

### 3.6.3–3.6.4 跳转指令与编码

- `jmp` — 无条件
- `je/jne/jg/jl/...` — 有条件（与 C `== != > <` 对应，注意有/无符号）
- **跳转目标：** 相对偏移（PC 相对）— 位置无关代码、ASLR 友好

### 3.6.5 条件分支

```c
if (a > b) x = a; else x = b;
// 典型：cmp; jle else; mov a; jmp end; else: mov b;
```

**分支预测失败** → 流水线清空 — 热路径上 **不可预测分支** 很贵。

### 3.6.6 条件传送 (cmov)

```c
// 编译器可能生成 cmov 而非分支
x = (a > b) ? a : b;
```

| | 分支 | cmov |
|---|------|------|
| 可预测分支 | 快 | 可能略慢 |
| **不可预测** | 很慢 | **常更稳** |
| 副作用 | 两侧可不同 | 两侧 **都会算** |

**HFT：** 热路径少分支；`likely/unlikely`、查表、位掩码；profile 看 **branch-misses**（→ [16-Systems-Performance Ch 6](../../../16-systems-performance/chapter-06-cpus/)）

### 3.6.7 循环

- `for`/`while` → `cmp` + `j` 回跳；`-O3` 可能 **展开 (unroll)**、向量化
- `do-while` 少一次前测跳转

### 3.6.8 switch

- 稀疏 case → 跳转表 `jmp *table(,%rax,8)`
- 密集小整数 → **O(1) 表跳转**；否则 if-else 链

**HFT：** 消息类型 dispatch 常用 **函数指针表 / switch** — 保证 case 值连续可生成跳转表。

### 自测题

<details>
<summary>1. `cmp` 和 `test` 指令的区别？各自设置什么 flags？</summary>

`cmp %a, %b` 计算 `b - a`（不存结果），设置 ZF/SF/CF/OF。用于比较大小。
`test %a, %b` 计算 `b & a`（不存结果），设置 ZF/SF（CF/OF 清零）。常用于 `test %rax, %rax`（检查是否为零/负）。

`test %rax, %rax` 比 `cmp $0, %rax` 更短（不需要立即数），编译器偏好 test。

</details>

<details>
<summary>2. 条件跳转 `je`/`jne`/`jl`/`jg` 分别在什么 flag 条件下跳转？</summary>

| 指令 | 跳转条件 | 含义 |
|---|---|---|
| je/jz | ZF=1 | 相等/为零 |
| jne/jnz | ZF=0 | 不等/非零 |
| jl/jnge | SF≠OF | 小于（有符号）|
| jg/jnle | ZF=0 且 SF=OF | 大于（有符号）|
| jb/jnae | CF=1 | 小于（无符号）|
| ja/jnbe | CF=0 且 ZF=0 | 大于（无符号）|

注意有符号(jl/jg)和无符号(jb/ja)用的 flag 不同——比较有符号和无符号混用是经典 bug。

</details>


---

← [本章导读](../README.md)
