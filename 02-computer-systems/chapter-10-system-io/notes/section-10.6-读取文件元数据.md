## 10.6 读取文件元数据

> **Ch10 §10.6** · [章导读](../README.md) · 上节 [§10.5 ←](./section-10.5-Rio包.md) · 下节 [§10.7 →](./section-10.7-读取目录内容.md)

---

```c
int stat(const char *pathname, struct stat *buf);
int fstat(int fd, struct stat *buf);
```

常用字段：

| 字段 | 含义 |
|------|------|
| `st_mode` | 类型 + 权限 |
| `st_size` | 字节大小 |
| `st_mtime` | 修改时间 |

```c
S_ISREG(st_mode)  // 普通文件
S_ISDIR(st_mode)  // 目录
```

**HFT：** 启动时 `stat` 配置文件；**mmap 前** 知 `st_size`；热路径避免 `stat`。

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§10.5 ←](./section-10.5-Rio包.md) · [本章导读](../README.md) · [§10.7 →](./section-10.7-读取目录内容.md)
