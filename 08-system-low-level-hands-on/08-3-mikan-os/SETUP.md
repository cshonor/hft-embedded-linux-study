# MikanOS · 环境搭建（Windows 主路径）

> 官方附录 A + [os-from-zero README](https://github.com/uchan-nos/os-from-zero) · 细节随笔记更新

## 推荐栈

| 组件 | 用途 |
|------|------|
| **WSL2** (Ubuntu 22.04+) | EDK II 构建、clang/gcc |
| **QEMU** + **OVMF** | UEFI 固件模拟（替代实体 U 盘调试） |
| **Git** | clone `uchan-nos/os-from-zero` |

## 快速验证（WSL）

```bash
sudo apt update
sudo apt install -y git build-essential clang lld xorriso mtools qemu-system-x86 ovmf
git clone https://github.com/uchan-nos/os-from-zero.git
cd os-from-zero
# 按仓库 README 构建 day02a 等 tag
```

## QEMU + OVMF（示例）

```bash
qemu-system-x86_64 \
  -bios /usr/share/OVMF/OVMF_CODE.fd \
  -drive format=raw,file=fat:rw:disk.img \
  -m 512M
```

> **与 08-1 差异：** 08-1 用 `qemu-system-i386 -fda`（软盘 BIOS）；MikanOS 用 **x86_64 + UEFI**。

## 路径建议

工程放 **纯英文路径**（与 [08-1 SETUP](../08-1-30days-os/SETUP.md) 相同约束），例如 `D:\dev\mikanos\` 或 WSL `~/dev/mikanos/`。

---

环境跑通后，在 [chapter-01-hello-uefi](./OUTLINE.md) 起记笔记。
