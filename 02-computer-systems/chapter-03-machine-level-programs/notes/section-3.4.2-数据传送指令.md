## 3.4.2–3.4.3 数据传送指令

> [章导读](../README.md) · 上节 [§3.4.1 操作数](./section-3.4.1-操作数指示符.md) · 下节 [§3.4.4 栈](./section-3.4.4-压栈与弹栈.md)

---

- `mov` — 源→目的（**不能** mem→mem）
- `movz` / `movs` — 零扩展 / 符号扩展加载较小类型
- `lea` — **只算地址、不访存**（也常用于 `x + k*scale` 快速算术）

### 自测题

<details>
<summary>1. `mov` 和 `lea` 的区别？什么时候用 lea？</summary>

`mov` 做**内存↔寄存器**的数据传送。`lea`（Load Effective Address）**计算地址但不访问内存**——只做算术。`lea (%rax,%rcx,4), %rdx` = `rdx = rax + rcx*4`，不读内存。

**lea 的妙用**：1. 快速乘法加法（`lea (%rax,%rax,2), %rdx` = rax×3）
2. 地址计算（数组索引）
3. 不影响 flags（和 add/mul 不同）

HFT 中 `lea` 常被编译器用于三操作数算术。

</details>

<details>
<summary>2. `movz` 和 `movs` 系列指令的区别？</summary>

`movz`（zero-extend）：高位补 0，用于**无符号**小→大转换。`movs`（sign-extend）：高位补符号位，用于**有符号**小→大转换。例：`movzbq %al, %rax`（byte→quad, 零扩展），`movsbq %al, %rax`（byte→quad, 符号扩展）。选错会导致负数变成大正数（有符号→无符号）或反之。

</details>


---

← [本章导读](../README.md) · [§3.4.1 ←](./section-3.4.1-操作数指示符.md) · [§3.4.4 →](./section-3.4.4-压栈与弹栈.md)
