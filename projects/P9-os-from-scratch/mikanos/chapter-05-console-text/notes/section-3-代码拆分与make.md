## 3. 代码拆分与 make

> 全部堆在 **`main.cpp`** → 难以维护 · **全量重编** 慢。

---

### 一、按职责拆分

| 模块 | 文件（示意） | 职责 |
|------|--------------|------|
| **图形** | `graphics.cpp` / `graphics.hpp` | PixelWriter、绘图原语 |
| **字体** | `font.cpp` / `font.hpp` | 字模、`WriteAscii` |
| **入口** | `main.cpp` | `KernelMain`、初始化顺序 |

```
main.cpp
    ├── #include "graphics.hpp"
    └── #include "font.hpp"
```

| 收益 | 说明 |
|------|------|
| **可读性** | 改字体不翻图形代码 |
| **编译速度** | **只重编变更的 .cpp** |

---

### 二、Makefile 增量编译

```makefile
kernel.elf: main.o graphics.o font.o hankaku.o
	$(LD) -o $@ $^ ...

main.o: main.cpp graphics.hpp font.hpp
graphics.o: graphics.cpp graphics.hpp
font.o: font.cpp font.hpp
```

| make 行为 | 效果 |
|-----------|------|
| 只改 `font.cpp` | 重编 **font.o** + 链接 |
| 头文件作依赖 | 改 `.hpp` 会触发包含它的 `.o` 重编 |

→ [Ch4 Makefile 入门](../chapter-04-pixel-make/notes/section-2-make与Makefile.md)

---

### 三、内核工程习惯

- **头文件声明 · 源文件实现** — 与 Linux 内核子系统同构（缩小版）
- **避免循环依赖** — graphics 不依赖 console（Console 在后续文件引入）

---

← [2. WriteAscii](./section-2-WriteAscii与位图字体.md) · 下一节 [4. 字体嵌入](./section-4-外部字体嵌入.md)
