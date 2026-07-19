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

---

← [§10.9 ←](./section-10.9-I-O重定向.md) · [本章导读](../README.md) · [§10.11 →](./section-10.11-综合该用哪些I-O？.md)
