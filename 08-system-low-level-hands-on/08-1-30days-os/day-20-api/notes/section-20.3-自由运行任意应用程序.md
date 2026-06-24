## ③ 自由运行任意应用程序 · `cmd_app`

之前：**只能敲固定命令**（如 **`hlt`**）。

**改 Console 解析：**

```
strcmp 内置命令 (mem/cls/dir/type…) 
    → 命中则执行
else
    cmd_app(输入行) → FAT 找 **同名.hrb** → load + run
```

**用户输入 `hello` → 运行 `hello.hrb`** — **Shell 即程序加载器**。

→ [Day 18/19 FAT + loadfile](../day-18-dir/)

---
