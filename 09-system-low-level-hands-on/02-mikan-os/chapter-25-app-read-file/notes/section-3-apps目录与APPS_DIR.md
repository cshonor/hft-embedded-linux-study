## 3. apps 目录与 APPS_DIR

---

### 一、根目录拥挤问题

**Ch18–24：** 应用增多 — **KERNEL.ELF · RPN · CUBE · BLOCKS …** 全在 **根**。

```
> ls
RPN     ELF  CUBE    ELF  BLOCKS  ELF  …  (满屏)
```

**可维护性差** — 与用户 **home/bin** 分离需求类似。

---

### 二、APPS_DIR 构建变量

**Makefile / 构建脚本：**

```makefile
APPS_DIR = apps
$(APPS_DIR)/%.elf: apps/%.cpp
    $(CXX) … -o $@
# 拷贝到 FAT 镜像时放入 apps/ 子目录
```

| 效果 | 说明 |
|------|------|
| **磁盘布局** | 所有 **用户 ELF** 在 **`/apps/`** |
| **终端启动** | `> apps/rpn` 或 **PATH 式** 习惯（本书仍 **显式路径**） |
| **ls 根目录** | 清爽 — **系统文件 vs 应用** 分离 |

---

### 三、与 noterm 配合

```
> noterm apps/cube
```

→ [Ch24 noterm](../chapter-24-multi-terminal/notes/section-4-窗口层级Bug与noterm.md)

---

← [2. 目录树/ls](./section-2-目录树与ls升级.md) · 下一节 [4. fd](./section-4-文件描述符与FileDescriptor.md)
