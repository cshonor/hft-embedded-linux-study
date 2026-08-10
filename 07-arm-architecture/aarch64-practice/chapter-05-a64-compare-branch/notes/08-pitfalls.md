# 5.8 易错点清单

> 来源：§5.8 · 精读 · [章总览](section-0-本章完整概述.md)

## 4 大易错点

1. **有符号 vs 无符号**比较后缀选错（LT/GE vs LO/HS）
2. **嵌套 BL 忘记保存 X30** → 返回地址被覆盖
3. **CBZ 只能判零**，不能比较大小 → 误用为通用比较
4. **CSEL 条件方向**搞反（GE 选谁？）

## 自测题

1. `csel x0, x1, x2, ge` — x0 最终是什么？
<details><summary>答案</summary>
如果 GE 条件成立（N==V），x0 = x1；否则 x0 = x2。注意 CSEL 的参数顺序：目的、条件真时选的源、条件假时选的源、条件。
</details>

2. 以下代码有什么 bug？
```asm
func:
    bl inner        ; 调用 inner
    bl inner2       ; 再次调用
    ret
```
<details><summary>答案</summary>
第一次 BL inner 会把返回地址存入 X30。第二次 BL inner2 又覆盖 X30。如果 inner 内部调用了其他函数（BL），X30 被进一步覆盖，func 的 RET 会跳到错误地址。必须在入口保存 X30：`stp x29, x30, [sp, #-16]!`。
</details>

3. 为什么 CBZ 不能替代 CMP+B.LT？
<details><summary>答案</summary>
CBZ 只判断是否等于零，不能判断大小关系。`B.LT` 需要先 CMP 设置 NZCV，然后根据 N≠V 跳转。CBZ 无法设置这些比较标志，只能做 ==0 或 !=0 的判断。
</details>

## 参考与延伸

- 原书 §5.8
- [5.3 跳转指令](03-branch.md)
- [3.4 STP/LDP](../../chapter-03-a64-load-store/notes/section-0-本章完整概述.md)
