## 10.10 标准 I/O (stdio)

> **Ch10 §10.10** · [章导读](../README.md) · 上节 [§10.9 ←](./section-10.9-I-O重定向.md) · 下节 [§10.11 →](./section-10.11-综合该用哪些I-O？.md)

---

```c
FILE *fopen(...);
size_t fread/fwrite(...);
char *fgets(...);
int fprintf/scanf(...);
```

- **`FILE*`** 带 **应用层缓冲** — 减少 syscall，但 **与 fd 层混用要小心**（`fflush`、重复缓冲）
- **线程安全** — `flockfile`；多线程热路径更倾向 **裸 fd + 自管缓冲**

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. C 标准库 stdio 和 Unix I/O 的关系？为什么有两层？</summary>

stdio(`fopen`/`fread`/`fprintf`) 在 Unix I/O(`open`/`read`/`write`) 之上封装：1. **缓冲管理**——减少系统调用次数
2. **格式化**——`printf`/`scanf` 解析格式字符串
3. **类型安全**——`FILE*` 抽象

两层的原因：Unix I/O 是系统级接口（fd），stdio 是语言级接口（FILE*）。stdio 内部用缓冲 + Unix I/O。**HFT 注意**：stdio 缓冲可能延迟输出（需 `fflush`），且 `FILE*` 有锁（`flockfile`），不适合超低延迟场景。

</details>


---

← [§10.9 ←](./section-10.9-I-O重定向.md) · [本章导读](../README.md) · [§10.11 →](./section-10.11-综合该用哪些I-O？.md)
