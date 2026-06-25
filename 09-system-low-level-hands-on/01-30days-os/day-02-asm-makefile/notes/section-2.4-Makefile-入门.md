## ④ Makefile 入门

步骤变多：汇编 → 拼映像 → 开模拟器 … 若全靠多个 **`.bat`**，目录 **又乱又难记**。

引入 **GNU Make** + **`Makefile`**：

```makefile
# 示意（非原书完整文件）
ipl.bin: helloos.nas
	nask helloos.nas helloos.lst ipl.bin

helloos.img: ipl.bin
	# 映像工具把 ipl.bin 写入引导扇区 …

run: helloos.img
	# 启动 QEMU …
```

命令行只打：

```bash
make        # 按依赖自动执行需要更新的步骤
make run    # 构建并运行（若规则已写）
```

| 对比 | `.bat` 堆叠 | **Makefile** |
|------|-------------|--------------|
| 依赖关系 | 手写顺序、易漏 | **目标–依赖–命令**，增量构建 |
| 目录 | 多个脚本 | **一个 Makefile** 收拢 |
| 习惯 | Windows 批处理 | 与 **Linux 内核 / HFT 工程** 同一套 |

**HFT：** 策略引擎、网关 **CMake / Make / Ninja** — 同一思想：**声明依赖，工具链自动增量编译**。

---
