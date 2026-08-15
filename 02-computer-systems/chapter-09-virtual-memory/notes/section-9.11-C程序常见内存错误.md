## 9.11 C 程序常见内存错误（精选）

> **Ch9 §9.11** · [章导读](../README.md) · 上节 [§9.10 ←](./section-9.10-垃圾收集.md) · 下节 [§9.12 →](./section-9.12-小结.md)

---

| 错误 | 后果 | 防范 |
|------|------|------|
| **坏指针解引用** | segfault | 初始化、检查 NULL |
| **读未初始化** | 随机行为 | `calloc`、值初始化 |
| **栈缓冲区溢出** | 安全漏洞 | 边界检查、`fgets`（→ [Ch 3](../../chapter-03-machine-level-programs/notes/section-3.10-指针调试与缓冲区溢出.md)） |
| **off-by-one** | 踩边界 | 循环 `< n`、分配 `n+1` |
| **指针/对象混淆** | 逻辑错 | `sizeof(*p)` vs `sizeof(p)` |
| **指针算术错** | 越界 | `p+i` 类型缩放 |
| **悬空引用** | UAF | 释放后置 NULL；Rust 编译期防 |
| **重复 free / 漏 free** | 崩溃/泄漏 | 所有权清晰；ASan/Valgrind |

```bash
# 开发期
gcc -fsanitize=address -g ...
valgrind --leak-check=full ./prog
```

**HFT：** CI 跑 **ASan/UBSan** 在测试二进制；生产靠 **代码规范 + 池化**；Rust 策略层减 UAF 类 bug。

---

### 常见陷阱
1. **ASan 有性能开销（2-5x），生产不能开** — ASan 在每次访存检查影子内存，开发/CI 开，生产关
2. **Valgrind 慢 10-50x，只用于测试** — 它模拟 CPU 执行，不修改二进制；ASan 编译期插桩
3. **UAF 比泄漏更危险** — 泄漏只是浪费内存；UAF 可能被利用（攻击者控制释放后的块内容）

### 自测题

<details>
<summary>Q1: ASan 和 Valgrind 的工作原理和性能开销有什么区别？</summary>

ASan：编译期插桩（-fsanitize=address），每次访存查影子内存，开销 2-5x，检测越界/UAF。Valgrind：运行期模拟 CPU 执行，不开编译选项，开销 10-50x，检测更全面（含未初始化读）。

</details>

<details>
<summary>Q2: 为什么 UAF（use-after-free）比内存泄漏更危险？</summary>

泄漏只是浪费内存，程序仍正确。UAF 访问已释放的内存，可能读到被重新分配的数据（逻辑错误），或被攻击者利用（控制释放块内容实现代码执行）。

</details>

<details>
<summary>Q3: HFT 在 CI 和生产中分别用什么工具检查内存错误？</summary>

CI：ASan + UBSan 编译测试二进制，Valgrind 跑集成测试。生产：不开 ASan（性能开销），靠代码规范 + 对象池 + RAII + Rust 策略层。

</details>

<details>
<summary>Q4: off-by-one 错误如何防范？</summary>

1) 循环用 `< n` 不是 `<= n`；2) 分配 `n+1` 字节给 n 字符串（留 '\0'）；3) 边界检查用 `fgets` 不用 `gets`；4) ASan 可检测栈/堆越界。

</details>

---

← [§9.10 ←](./section-9.10-垃圾收集.md) · [本章导读](../README.md) · [§9.12 →](./section-9.12-小结.md)
