## 6. ls 命令与小结

---

### 一、挂到 ExecuteLine()

**Ch16 命令分派** 扩展：

```cpp
if (strcmp(cmd, "ls") == 0) {
    ListRootDirectory();
}
```

| 步骤 | 说明 |
|------|------|
| 1 | 用 **BPB** 找 **根目录起始簇** |
| 2 | 内存地址 → **`DirectoryEntry*`** |
| 3 | 循环 **32B** — 过滤空槽 / LFN / 已删 |
| 4 | **格式化 8.3** → 终端 **Print** |

**示例输出：**

```
> ls
KERNEL  ELF
APPS    DIR
README  TXT
>
```

---

### 二、调试要点

| 现象 | 排查 |
|------|------|
| **空列表** | BPB 偏移错 · **根目录簇** 非 2 · 镜像未传入 |
| **乱码名** | 小端簇号 · **8.3 未 trim 空格** |
| **缺文件** | 仅 **LFN** 无短名 · 预读 **16MiB 不够** |

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **FAT 概念 + BPB** | 卷 **几何与根目录定位** |
| **Directory Entry** | **8.3 · 属性 · 起始簇** |
| **Block I/O 预读** | Bootloader **块读** → Kernel **纯内存解析** |
| **`ls`** | **持久化数据** 首次在 CLI **可见** |

```
Ch17 列目录
    ↓
Ch18 读 .elf 运行用户程序
Ch20 syscall 抽象文件 I/O
```

---

### 四、后续索引

| Ch17 主题 | 继续读 |
|----------|--------|
| 加载应用 | [chapter-18-apps](../chapter-18-apps/) ⚪ |
| 系统调用 | [chapter-20-syscall](../chapter-20-syscall/) 🔴 |
| UEFI 引导 | [chapter-01-hello-world](../chapter-01-hello-world/) |
| CLI 框架 | [chapter-16-commands](../chapter-16-commands/) |

---

← [5. Block I/O](./section-5-UEFI-Block-IO与卷镜像.md) · [Ch 16](../chapter-16-commands/) · [Ch 17 导读](../README.md)
