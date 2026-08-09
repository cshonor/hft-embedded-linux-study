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

### 自测题

<details>
<summary>1. `opendir`/`readdir` 和 `scandir` 的区别？</summary>

`opendir` → `readdir`(循环) → `closedir`：逐个读目录项，返回 `struct dirent`（含 `d_name`）。
`scandir(path, &namelist, filter, compar)`：一次性读取所有匹配的目录项，排序后返回。

目录的内容是文件名→inode 的映射表，`readdir` 逐行扫描这张表。HFT 很少直接读目录（热路径不用），但启动时加载配置文件/插件目录会用到。

</details>


---

← [§10.6 ←](./section-10.6-读取文件元数据.md) · [本章导读](../README.md) · [§10.8 →](./section-10.8-共享文件.md)
