## 2. 目录树与 ls 升级

---

### 一、目录 = 特殊文件

**FAT：** 目录内容 = **Directory Entry 数组**（Ch17）— 条目可指向 **子目录簇**。

```
/
├── APPS    DIR  →  cluster N
│   ├── RPN     ELF
│   └── GREP    ELF
└── README  TXT
```

---

### 二、fat::FindFile() 递归

**Ch17：** 多在 **根目录** 按 **8.3 短名** 查找。

**Ch25 重写：**

```cpp
// 解析 "apps/grep" 或 "/apps/grep"
std::optional<DirectoryEntry> FindFile(std::string_view path) {
    auto components = Split(path, '/');
    ClusterId dir = root_cluster;
    for (size_t i = 0; i + 1 < components.size(); ++i) {
        auto e = FindEntryInCluster(dir, components[i]);
        if (!e || !e->is_directory()) return nullopt;
        dir = e->first_cluster();
    }
    return FindEntryInCluster(dir, components.back());
}
```

| 能力 | 示例 |
|------|------|
| **多级路径** | `ls apps` · `cat apps/readme.txt` |
| **Walk 簇链** | 子目录 **大目录** 跨簇 |

→ [Ch17 目录项](../chapter-17-filesystem/notes/section-4-目录条目结构.md) · [Ch18 簇链](../chapter-18-apps/notes/section-2-FAT簇链与cat命令.md)

---

### 三、ls 命令升级

```
> ls
> ls apps
> ls /apps
```

**ExecuteLine：** 参数作为 **路径** 传给 **ListDirectory(path)** — 不再 **仅根目录**。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. apps/](./section-3-apps目录与APPS_DIR.md)
