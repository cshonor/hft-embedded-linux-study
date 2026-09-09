# 02 · 交叉工具链：为什么不能在板子上编译

> **本节讲什么：** 建立 aarch64 交叉工具链，理解 target triple / sysroot / ABI 三件事，并能在 x86 主机上产出板子能跑的二进制。
> **对应动手：** [P5 · D1](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（交叉编译 C/C++ 部署到板）

---

## 要点

| 概念 | 说明 |
|------|------|
| **为什么必须交叉** | 目标板算力弱、存储小、无开发环境；且编译器本身要跑在宿主机 |
| **target triple** | `aarch64-linux-gnu` = 架构(aarch64) - 厂商(none) - OS(linux) - ABI(gnu) |
| **sysroot** | 目标板的"根文件系统副本"——头文件和库从这里找，而不是宿主机的 `/usr/include`。**弄错 sysroot 是最常见的隐性错误** |
| **ABI 匹配** | 浮点调用约定（hard-float）、`sizeof(long)`、结构体对齐必须与目标内核一致，否则能跑但结果错 |
| **验证** | `file` 看 ELF 架构 + `readelf -h` 看 Machine，比"能编译过"更可靠 |

---

## 关键命令

```bash
# 装工具链（Ubuntu/WSL）
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 编译 + 检查产物
aarch64-linux-gnu-gcc -O2 -o hello hello.c
file hello                      # ELF 64-bit LSB executable, ARM aarch64
aarch64-linux-gnu-readelf -h hello | grep Machine

# 看它到底链了哪些库
aarch64-linux-gnu-readelf -d hello | grep NEEDED
```

**坑：** 用 `gcc` 编出 x86 二进制再 scp 到板子上跑，会报 `Exec format error`——这不是权限问题。

---

## HFT / 嵌入式关联

- 生产环境虽然同为 x86，但**交叉/远程构建**的思路一致：构建机与运行机分离，产物必须可复现（`-frecord-gcc-switches`、固定 sysroot）。
- `sysroot` 的概念直接对应后面 [05-rootfs](../05-rootfs/) 里的"根文件系统必须先于应用存在"。

---

## 验收

- [ ] `aarch64-linux-gnu-gcc` 能编出 `file` 认得出的 aarch64 ELF
- [ ] 能解释 sysroot 的作用，说出它与宿主机 `/usr` 的区别
- [ ] 静态链接 vs 动态链接各编一次，对比 `NEEDED` 差异
- [ ] 产物 scp 到 Pi 5 上能跑

---

## 衔接

- **上一步：** [01-orientation](../01-orientation/)
- **下一步：** [03-u-boot](../03-u-boot/)
- **卡住查书：** MELP Ch2 · Primer Ch12–13
