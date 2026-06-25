## 关键术语

#### IPL（Initial Program Loader · 启动程序加载器）

- 磁盘 **最开头** 的 **启动区只有 512 字节**，装不下完整 OS。
- 这段小程序的任务：**把 OS 本体再加载进内存** → 叫 **IPL**。
- Day 1 的 `helloos` 已是「能跑的最小 IPL」；后面 Day 会扩展成 **真正的加载器**。

#### Boot（启动 · bootstrap）

- 来自 **bootstrap（靴带）** — 「拽着靴带把自己提起来」的 **逻辑悖论**：
  - 要运行 OS，得先有程序 **从磁盘读 OS 进内存**；
  - 但 **读 OS 的程序本身也在磁盘里** — 谁先来？
- 硬件约定：**上电 → BIOS/UEFI 读引导扇区 → 执行 IPL** — 这条 **自力更生的链** 叫 **bootstrap**，简称 **boot**。

→ 与 Linux 启动链对照：[05-LKD](../../../../05-Linux-Kernel-Development/) · GRUB / `vmlinuz` 是 **多阶段 IPL** 的工业版。

---
