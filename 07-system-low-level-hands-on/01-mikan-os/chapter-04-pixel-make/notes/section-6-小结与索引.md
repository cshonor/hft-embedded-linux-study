## 6. 小结与索引

---

### 一、本章总结

| 成果 | 说明 |
|------|------|
| **Makefile** | 内核构建自动化 |
| **PixelWriter** | RGB/BGR · **虚函数** 去掉像素循环内分支 |
| **vtable / Placement new** | 多态底层 · **无堆** 构造对象 |
| **ELF LOAD** | 修正 Loader — **范围计算 + 段拷贝 + 入口** |

```
工程层：make
    +
图形层：PixelWriter → 任意颜色像素
    +
Loader层：正确 PT_LOAD — 内核映像可靠
    ↓
Ch 5 文本 / Ch 10 窗口 …
```

---

### 二、与前后章关系

| 章 | 关系 |
|----|------|
| **Ch 3** | GOP + ELF 初版 → Ch 4 **完善绘图与加载** |
| **Ch 5** | 在像素之上叠 **字体与控制台** |
| **Ch 8** | 堆分配器 → 普通 `new` 才安全 |

---

### 三、后续索引

| Ch4 主题 | 继续读 |
|----------|--------|
| 控制台文本 | [chapter-05-console-text](../chapter-05-console-text/) ⚪ |
| 窗口 | [chapter-10-window](../chapter-10-window/) ⚪ |
| C++ 模板 | [appendix-D-cpp-templates](../../appendix-D-cpp-templates/) |
| 分页 / 地址 | [chapter-19-paging](../chapter-19-paging/) 🔴 |

---

← [5. ELF 加载器](./section-5-ELF格式与加载器改进.md) · [Ch 3](../chapter-03-bootloader-display/) · [Ch 4 导读](../README.md)
