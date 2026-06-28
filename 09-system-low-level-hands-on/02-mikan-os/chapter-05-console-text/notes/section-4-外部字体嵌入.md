## 4. 外部字体嵌入（東雲 / objcopy）

> **尚无文件系统** — 不能运行时从磁盘读 `.bmp` / 字体文件。

---

### 一、需求

| 初版 8×8 点阵 | **東雲（Hankaku）开源字体** |
|---------------|----------------------------|
| 仅简单 ASCII | **半角日文等更丰富 glyph** |
| C 数组手写 | 数据量大 — 宜 **工具生成** |

---

### 二、嵌入流程（概念）

```
字体源数据（二进制 / 由 Python 生成）
    ↓
objcopy 转为 ELF 对象 hankaku.o
    ↓
链接进 kernel.elf — 符号如 _binary_hankaku_font_start
    ↓
KernelMain 直接使用 — 无需读盘
```

| 工具 | 作用 |
|------|------|
| **Python 脚本** | 格式转换、生成中间二进制 |
| **`objcopy -I binary -O elf64-x86-64`** | 裸二进制 → **可链接 .o** |
| **链接器** | 把 `.o` 并入内核，暴露 **起止符号** |

**C/C++ 侧：**

```cpp
extern "C" {
extern const uint8_t _binary_hankaku_font_start[];
extern const uint8_t _binary_hankaku_font_end[];
}
// 字模偏移 = 在 [start, end) 内按 glyph 索引
```

| 优点 | 说明 |
|------|------|
| **启动即可用** | 不依赖 Ch 17 文件系统 |
| **只读 .rodata** | 链入 **只读段** — 与 PT_LOAD 一致 |

---

### 三、与后续 FS 章的关系

| 阶段 | 字体来源 |
|------|----------|
| **Ch 5（本章）** | **链接时嵌入** |
| **Ch 17+** | 可从磁盘加载 — 可选替换 |

---

← [3. 代码拆分](./section-3-代码拆分与make.md) · 下一节 [5. Console](./section-5-Console与Newlib.md)
