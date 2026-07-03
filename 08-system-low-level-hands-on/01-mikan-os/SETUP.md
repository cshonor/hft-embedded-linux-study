# MikanOS · 环境搭建（Windows 主路径）

> 官方附录 A + [os-from-zero README](https://github.com/uchan-nos/os-from-zero) · 细节随笔记更新

## 推荐栈

| 组件 | 用途 |
|------|------|
| **WSL2** (Ubuntu 22.04+) | **主路径**：Windows 本机 + Linux 编译环境 |
| **llvm + lld** | **Ch1 推荐** — 纯 LLVM；`make LINK=ld.lld` |
| **Clang + lld-link** | 备选 — 对齐官方 day01/c |
| **x86_64-elf-gcc** | 可选 — 仅跟官方 mikanos-build `buildenv.sh` 时需要 |
| **QEMU** + **OVMF** | UEFI 固件模拟 |
| **Git** | clone 官方仓库（可选） |

## Ch1 最快路径（本仓库 · 不必先 clone 官方）

**只装 Clang 线**，进笔记配套 `code/` 即可：

```bash
sudo apt update
sudo apt install -y llvm lld qemu-system-x86 ovmf
cd …/chapter-01-hello-world/code
make LINK=ld.lld run
```

→ [chapter-01-hello-world/code/](./chapter-01-hello-world/code/README.md)

## 全书 MikanOS 验证（可选 · 稍后）

```bash
sudo apt install -y git build-essential clang lld xorriso mtools qemu-system-x86 ovmf
# x86_64-elf 工具链：见 mikanos-build 发布包 + buildenv.sh
git clone https://github.com/uchan-nos/os-from-zero.git
```

## QEMU + OVMF（示例）

```bash
qemu-system-x86_64 \
  -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive format=raw,file=fat:rw:disk.img \
  -m 512M
```

> **与 01 差异：** 01 用 `qemu-system-i386 -fda`（软盘 BIOS）；MikanOS 用 **x86_64 + UEFI**。

## 路径建议

工程放 **纯英文路径**（与 [01 SETUP](../02-30days-os/SETUP.md) 相同约束），例如 `D:\dev\mikanos\` 或 WSL `~/dev/mikanos/`。

---

环境跑通后，在 [chapter-01-hello-world](./chapter-01-hello-world/) 起记笔记。
