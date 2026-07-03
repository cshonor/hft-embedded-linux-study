## 3. 真机与 QEMU 测试

---

### 一、真机启动

**流程：** U 盘制作完成 → 重启 → 选择 **USB 启动** → UEFI 加载 `BOOTX64.EFI`。

**若无法启动 — 常见原因：Secure Boot**

| 项 | 说明 |
|----|------|
| **Secure Boot** | UEFI 安全功能 — **验证引导镜像签名**，拦截未签名/自制 EFI |
| **解决** | 进入 **BIOS/UEFI 设置** → **禁用 Secure Boot**（书名场景下的常见必要步骤） |
| **注意** | 生产机器上禁用有安全含义；**学习用机** 或 **专用测试机** 更合适 |

其他检查：USB 启动顺序、U 盘是否 **GPT/MBR + FAT** 正确、文件名大小写（通常不敏感但路径须对）。

---

### 二、QEMU 模拟器（推荐日常开发）

**优势：** 无需反复插拔 U 盘；可脚本化；与 [SETUP.md](../../SETUP.md) WSL 栈一致。

官方仓库提供 **`run_qemu.sh`** 一类脚本 — 典型要素：

```bash
qemu-system-x86_64 \
  -bios OVMF_CODE.fd \          # UEFI 固件（OVMF）
  -drive format=raw,file=fat:rw:disk.img \   # 内含 EFI/BOOT/BOOTX64.EFI
  -m 512M
```

| 与 01 差异 | 说明 |
|------------|------|
| **02 川合** | `qemu-system-i386 -fda helloos.img` — **BIOS 读软盘第 0 扇区 IPL** |
| **MikanOS** | **x86_64 + OVMF** — **UEFI 读 FAT 上的 BOOTX64.EFI 文件**（不是 `-fda` 引导扇区） |

→ 环境依赖见 [appendix-A-dev-env](../../appendix-A-dev-env/) · [SETUP.md](../../SETUP.md)

---

### 三、真机 vs QEMU 选型

| | **真机** | **QEMU** |
|---|----------|----------|
| **体感** | 真实硬件时序、USB/显示 quirks | 快速迭代、可 gdb |
| **用途** | 最终验证、USB/显卡差异 | **日常编译-运行循环** |
| **Secure Boot** | 常需手动关 | OVMF 通常 **无 Secure Boot 拦截**（视配置） |

**建议：** 开发以 **QEMU 为主**，里程碑再在真机复验。

---

← [2. C + Makefile](./section-2-二进制编辑器与BOOTX64.md) · [01-clang-minimal/](../code/01-clang-minimal/) · 下一节 [4. 结构与编码](./section-4-计算机结构与编码.md)
