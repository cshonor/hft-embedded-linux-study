## 2. FindCommand 与 PATH 搜索

---

### 一、痛点

**Ch25 起：** 应用统一在 **`apps/`** — 启动需：

```
> apps/grep pattern file
```

**繁琐** — 与 **Linux PATH** 体验差距大。

---

### 二、FindCommand()

```cpp
std::optional<std::string> FindCommand(std::string_view name) {
    // 1. 若含 '/' — 按路径直接 FindFile
    if (name.contains('/'))
        return std::string(name);
    // 2. 否则在 apps/ 下查找 name / NAME.ELF
    if (auto e = fat::FindFile("apps/" + name)) …
    if (auto e = fat::FindFile("apps/" + ToUpper8_3(name)))
        return "apps/" + …;
    return nullopt;
}
```

**ExecuteLine / CallApp：**

```
> grep hoge memmap
FindCommand("grep") → "apps/grep"
→ 加载 ELF
```

| 类比 | Linux **`PATH=/apps`**（本书 **硬编码 apps/**） |
|------|--------------------------------------------------|

→ [Ch25 APPS_DIR](../chapter-25-app-read-file/notes/section-3-apps目录与APPS_DIR.md) · [Ch18 fallback 执行](../chapter-18-apps/notes/section-3-无头二进制与磁盘执行.md)

---

### 三、内置 vs 外部

| 类型 | 解析 |
|------|------|
| **内置** echo/ls/cat/… | **先匹配** 内置表 |
| **外部** | **FindCommand** → **ELF** |

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. more](./section-3-more命令与管道按键.md)
