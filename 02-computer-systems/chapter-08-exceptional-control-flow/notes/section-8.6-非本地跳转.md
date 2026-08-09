## 8.6 非本地跳转

> **Ch8 §8.6** · [章导读](../README.md) · 上节 [§8.5 ←](./section-8.5-信号.md) · 下节 [§8.7 →](./section-8.7-操作进程的工具.md)

---

```c
#include <setjmp.h>
jmp_buf env;
if (setjmp(env) == 0) {
    // 正常路径
} else {
    // longjmp(env, 1) 跳回此处
}
longjmp(env, 1);
```

- **不还原栈展开** — 与 C++ **异常、RAII** 不兼容；**跳过析构**
- 用途：深层错误快速回退（老代码）；现代 C++ 用 **异常** 或 **Result 类型**

**HFT：** 新代码 **避免 setjmp**；协程/状态机用显式枚举更清晰。

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. setjmp/longjmp 的作用是什么？它和 goto 有什么区别？</summary>

`setjmp` 保存当前寄存器/栈上下文到一个 `jmp_buf`，`longjmp` 恢复该上下文——实现跨函数跳转。**和 goto 的区别**：goto 只能函数内跳转，longjmp 可以跨多层函数调用栈「弹回」。常用于错误恢复（如深层解析失败直接跳回主循环）。

</details>

<details>
<summary>2. longjmp 后局部变量的值是否可靠？</summary>

**不可靠**。`setjmp` 之后、`longjmp` 之前被修改的非 `volatile` 局部变量，在 `longjmp` 后的值是**未定义**的（编译器可能把变量优化到寄存器，longjmp 恢复的是旧寄存器值）。解决：对需要在 longjmp 后保留的局部变量加 `volatile`。

</details>


---

← [§8.5 ←](./section-8.5-信号.md) · [本章导读](../README.md) · [§8.7 →](./section-8.7-操作进程的工具.md)
