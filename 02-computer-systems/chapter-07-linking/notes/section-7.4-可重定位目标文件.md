## 7.4 可重定位目标文件

> **Ch7 §7.4** · [章导读](../README.md) · 上节 [§7.3 ←](./section-7.3-目标文件.md) · 下节 [§7.5 →](./section-7.5-符号和符号表.md)

---

ELF `.o` 典型节 (sections)：

| 节 | 内容 |
|----|------|
| `.text` | 已编译机器码 |
| `.data` | 已初始化全局/静态变量 |
| `.bss` | 未初始化全局（仅占位，不占文件字节） |
| `.symtab` | 符号表 |
| `.rel.text` / `.rel.data` | **重定位条目** |
| `.debug_*` | 调试信息（`-g`） |

- **节 (section)** — 链接器视角；**段 (segment)** — 加载器视角（`readelf -l`）

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. ELF 可重定位目标文件包含哪些 section？</summary>

关键 section：`.text`（代码）、`.rodata`（只读数据）、`.data`（已初始化全局变量）、`.bss`（未初始化全局变量，不占文件空间）、`.symtab`（符号表）、`.strtab`（字符串表）、`.rel.text`（代码重定位条目）、`.rel.data`（数据重定位条目）。

`readelf -S` 查看 section 表，`objdump -d` 反汇编 `.text`。

</details>


---

← [§7.3 ←](./section-7.3-目标文件.md) · [本章导读](../README.md) · [§7.5 →](./section-7.5-符号和符号表.md)
