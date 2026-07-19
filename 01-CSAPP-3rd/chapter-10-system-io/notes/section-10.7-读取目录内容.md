## 10.7 读取目录内容

> **Ch10 §10.7** · [章导读](../README.md) · 上节 [§10.6 ←](./section-10.6-读取文件元数据.md) · 下节 [§10.8 →](./section-10.8-共享文件.md)

---

```c
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
```

- 扫描配置目录、日志轮转 — 非 tick 路径

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§10.6 ←](./section-10.6-读取文件元数据.md) · [本章导读](../README.md) · [§10.8 →](./section-10.8-共享文件.md)
