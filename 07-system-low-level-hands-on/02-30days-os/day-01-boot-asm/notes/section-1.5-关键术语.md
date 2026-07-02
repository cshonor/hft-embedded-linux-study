## 关键术语

#### IPL（Initial Program Loader · 启动程序加载器）

- 磁盘 **最开头** 的 **启动区只有 512 字节**，装不下完整 OS。
- 在 VS Code 里写的 **`helloos.asm`** 就是这段启动区 **源码** — 整盘 OS 的 **「敲门砖」**；**NASM** 用 **`nasm -f bin helloos.asm -o ipl.bin`** 编成 **`ipl.bin`**（512 B 启动区镜像）。
- **上电后 BIOS 先读这 512 字节到内存**（常见 **`0x7C00`**），再把 **CPU 控制权交给 IPL**；IPL 的任务才是 **把 OS 本体从磁盘加载进内存**（Day 1 的 `helloos` 只演示输出，后面 Day 扩展成 **真正的加载器**）。
- 工程上 **IPL 与 1.44 MB 整盘分离** 构建 — 见 [Day 2 §2.3](../../day-02-asm-makefile/notes/section-2.3-先制作启动区.md)。

#### Boot（启动 · bootstrap）

- 来自 **bootstrap（靴带）** — 「拽着靴带把自己提起来」的 **逻辑悖论**：
  - 要运行 OS，得先有程序 **从磁盘读 OS 进内存**；
  - 但 **读 OS 的程序本身也在磁盘里** — 谁先来？
- 硬件约定：**上电 → BIOS/UEFI 读引导扇区 → 执行 IPL** — 这条 **自力更生的链** 叫 **bootstrap**，简称 **boot**。

→ 与 Linux 启动链对照：[03-Linux-Kernel-Development](../../../../03-Linux-Kernel-Development/) · GRUB / `vmlinuz` 是 **多阶段 IPL** 的工业版。

---
