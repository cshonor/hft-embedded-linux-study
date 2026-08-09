## 7.14 处理目标文件的工具

> **Ch7 §7.14** · [章导读](../README.md) · 上节 [§7.13 ←](./section-7.13-库打桩.md) · 下节 [§7.15 →](./section-7.15-小结.md)

---

| 工具 | 用途 |
|------|------|
| `ar` | 创建/查看静态库 |
| `nm` | 符号表 |
| `objdump -d/-r/-t` | 反汇编、重定位、符号 |
| `readelf -a` | ELF 头、节、段、动态段 |
| `size` | 各段大小 |
| `strings` | 可打印串 |
| `ldd` | 动态依赖 |
| `strip` | 去符号减体积 |

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. 列出处理 ELF 目标文件的常用工具及其用途。</summary>

| 工具 | 用途 |
|---|---|
| `ar` | 创建/查看静态库(.a) |
| `strings` | 查看二进制中的字符串 |
| `strip` | 删除符号表（减小文件）|
| `nm` | 查看符号表 |
| `readelf` | 查看 ELF 头/section/segment |
| `objdump` | 反汇编 |
| `ldd` | 查看动态库依赖 |
| `objcopy` | 转换/修改目标文件 |

HFT 调试：`nm -C a.out | grep symbol` 查符号，`objdump -d -M intel` 反汇编（Intel 语法），`readelf -r` 查看重定位表。

</details>


---

← [§7.13 ←](./section-7.13-库打桩.md) · [本章导读](../README.md) · [§7.15 →](./section-7.15-小结.md)
