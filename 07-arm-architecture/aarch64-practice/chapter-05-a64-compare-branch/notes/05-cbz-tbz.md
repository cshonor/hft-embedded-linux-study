# 5.5 CBZ / CBNZ / TBZ / TBNZ

> 来源：§5.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

CBZ/CBNZ（比较为零跳转）和 TBZ/TBNZ（测试位为零跳转）—— AArch64 专用条件跳转指令，将比较和跳转合并为一条指令。

## 核心要点

### 指令全览

| 指令 | 全称 | 作用 | 等价两条指令 | 跳转范围 |
|------|------|------|-------------|----------|
| CBZ Xn, label | Compare and Branch if Zero | Xn==0 则跳转 | CMP Xn,#0; B.EQ | ±1MB |
| CBNZ Xn, label | Compare and Branch if Not Zero | Xn!=0 则跳转 | CMP Xn,#0; B.NE | ±1MB |
| TBZ Xn, #bit, label | Test Bit and Branch if Zero | Xn[bit]==0 则跳转 | TST Xn,#(1<<bit); B.EQ | ±32KB |
| TBNZ Xn, #bit, label | Test Bit and Branch if Not Zero | Xn[bit]!=0 则跳转 | TST Xn,#(1<<bit); B.NE | ±32KB |

### CBZ / CBNZ 详解

```asm
; CBZ：如果 x0 == 0 则跳转
CBZ x0, null_handler     ; if (x0 == 0) goto null_handler

; CBNZ：如果 x0 != 0 则跳转
CBNZ x0, process         ; if (x0 != 0) goto process

; 与 CMP+B.EQ/B.NE 的等价关系
; CBZ x0, label  ≡  CMP x0, #0; B.EQ label
; CBNZ x0, label ≡  CMP x0, #0; B.NE label
```

**CBZ/CBNZ 的优势**：
1. 1 条指令 vs 2 条 → 代码更紧凑
2. **不修改 NZCV 标志** → 不影响后续条件判断
3. 更大的跳转范围（±1MB vs B.cond 的 ±1MB，相同但比 TBZ 大）

### TBZ / TBNZ 详解

```asm
; TBZ：如果 x0 的第 N 位 == 0 则跳转
TBZ x0, #7, not_signed    ; if (x0[7] == 0) goto not_signed（字节正数）
TBZ x0, #63, not_negative ; if (x0[63] == 0) goto not_negative（64位正数）

; TBNZ：如果 x0 的第 N 位 != 0 则跳转
TBNZ x0, #63, negative    ; if (x0[63] != 0) goto negative（符号位为1=负数）
TBNZ x0, #0, odd_number   ; if (x0[0] != 0) goto odd_number（最低位=奇数）

; 与 TST+B.EQ/B.NE 的等价关系
; TBZ x0, #5, label  ≡  TST x0, #(1<<5); B.EQ label
; TBNZ x0, #5, label ≡  TST x0, #(1<<5); B.NE label
```

**位号编码**：
- 32 位寄存器（Wn）：位号范围 0-31
- 64 位寄存器（Xn）：位号范围 0-63
- 位号在指令中直接编码为立即数（5-6 位）

### TBZ/TBNZ 的优势

```asm
; 传统方式：测试某一位
MOV x1, #(1 << 5)        ; 构造掩码
TST x0, x1               ; AND 操作，设 Z 标志
B.EQ bit_is_zero         ; 如果 Z=1（AND 结果为0）→ 该位为0
; 共 3 条指令，需要 1 个临时寄存器

; TBZ 方式：1 条指令
TBZ x0, #5, bit_is_zero  ; 直接测试第5位
; 1 条指令，不需要临时寄存器，不修改 NZCV
```

### 典型应用：判零与判空

```asm
; 空指针检查
func:
    CBZ x0, null_error       ; if (ptr == NULL) goto error
    LDR x1, [x0]             ; 安全解引用
    ...

; 链表遍历
loop:
    CBZ x0, end              ; if (node == NULL) → 遍历结束
    LDR x0, [x0, #8]         ; node = node->next
    B loop
end:
```

### 典型应用：标志位测试

```asm
; 状态寄存器解析（如页表项）
LDR x1, [page_table_entry]

; 测试有效位（bit 0）
TBNZ x1, #0, valid_page     ; bit0=1 → 页面有效
; bit0=0 → 无效页面
B fault_handler

valid_page:
; 测试可写位（bit 1）
TBZ x1, #1, read_only       ; bit1=0 → 只读
; bit1=1 → 可写

; 测试用户/内核位（bit 6）
TBNZ x1, #6, user_page      ; bit6=1 → 用户页
; bit6=0 → 内核页
```

### 典型应用：奇偶判断

```asm
; 判断 x0 是奇数还是偶数
TBZ x0, #0, even            ; bit0=0 → 偶数
; bit0=1 → 奇数
; 比 CMP+AND+B 更简洁
```

### 跳转范围对比

```
CBZ/CBNZ：19 位偏移 × 4 = ±1MB     ← 适合函数内跳转
TBZ/TBNZ：14 位偏移 × 4 = ±32KB    ← 只适合短距离跳转

如果 TBZ 目标超出 ±32KB：
  TBZ x0, #5, near_label    ; 短跳到中转点
  B continue
near_label:
  B far_label               ; 无条件 B 范围 ±128MB
  continue:
```

## 与 C 的对照

| C 代码 | AArch64 汇编 |
|--------|-------------|
| `if (ptr == NULL)` | `CBZ x0, label` |
| `if (ptr != NULL)` | `CBNZ x0, label` |
| `if (flags & (1 << n))` | `TBNZ x0, #n, label` |
| `if (!(flags & (1 << n)))` | `TBZ x0, #n, label` |
| `if (x < 0)` | `TBNZ x0, #63, label`（或 `CMP x0,#0; B.MI`）|
| `if (x % 2 == 1)` | `TBNZ x0, #0, label` |

## 常见错误

1. **CBZ 只能判零**：CBZ 只能判断 ==0 或 !=0，不能比较大小。`CBZ x0, label` 不是 `if (x0 < 10)`。
2. **TBZ 位号超范围**：64 位寄存器位号 0-63，32 位寄存器位号 0-31。TBZ w0, #63 编译报错。
3. **TBZ 跳转范围太小**：±32KB 可能不够，需要中转。

## HFT 关联

这些指令在空指针检查和标志测试中极其常用：
- `cbz x0, error` 空指针检查 → 1 条指令
- `tbnz x0, #flag_bit, handler` 测试状态标志位 → 1 条指令
- 比 CMP+TST+B 组合少 1 条指令 → 减少代码体积和延迟
- 编译器自动使用这些指令优化 if(ptr==NULL) 和 if(flags & MASK) 模式
- **不修改 NZCV** → 可以在 CMP 后插入 CBZ/TBZ 而不影响后续 B.cond

```asm
; HFT：先 CMP 设标志，再用 CBZ 不影响标志，最后用 B.cond
CMP x0, min_threshold        ; 设 NZCV
CBZ x1, skip_aux             ; x1==0 跳过辅助检查，不影响 NZCV！
; 这里 NZCV 仍然是 CMP 的结果
B.LT below_threshold         ; x0 < min_threshold
skip_aux:
```

## 自测题

1. `CBZ x0, label` 和 `CMP x0, #0; B.EQ label` 哪个更好？
<details><summary>答案</summary>
CBZ 更好：1 条指令 vs 2 条，更紧凑。CBZ 不修改 NZCV 标志（不执行 CMP），避免影响后续条件判断。
</details>

2. 如何测试 x0 的第 15 位是否为 1？
<details><summary>答案</summary>
```asm
tbnz x0, #15, bit_set   ; 如果 bit[15]=1 则跳转
```
TBNZ 直接测试特定位，不需要 AND 构造掩码。
</details>

3. CBZ 和 TBZ 的跳转范围有何区别？
<details><summary>答案</summary>
CBZ/CBNZ 的跳转范围约 ±1MB（19 位偏移×4）。TBZ/TBNZ 的跳转范围约 ±32KB（14 位偏移×4）。TBZ 范围更小因为指令中要编码位号。
</details>

4. 为什么说 CBZ "不修改 NZCV" 很重要？举一个具体例子。
<details><summary>答案</summary>
因为可以在 CMP 之后插入 CBZ 做辅助判断，而不破坏 CMP 设置的标志：
```asm
CMP x0, x1          ; 设 NZCV
CBZ x2, skip        ; x2==0 则跳过，不影响 NZCV
B.GT greater        ; 仍然使用 CMP 设置的 NZCV
skip:
```
如果用 `CMP x2, #0; B.EQ skip` 替代 CBZ，CMP 会覆盖 NZCV，后续 B.GT 会用错误的标志。
</details>

5. 用 TBZ 判断 x0（64位有符号整数）是否为负数。
<details><summary>答案</summary>
```asm
TBNZ x0, #63, negative   ; bit[63]=1 → 负数
; 或者
TBZ x0, #63, non_negative ; bit[63]=0 → 非负
```
64 位有符号数的符号位是第 63 位。TBZ/TBNZ 直接测试这一位，比 `CMP x0, #0; B.MI` 更简洁（1 条指令 vs 2 条）。
</details>

## 参考与延伸

- 原书 §5.5
- [5.1 比较指令](01-compare.md)
- [4.4 位操作](../../chapter-04-a64-arithmetic-shift/notes/04-bit-ops.md)
- [4.5 位段提取](../../chapter-04-a64-arithmetic-shift/notes/05-bit-field.md)
