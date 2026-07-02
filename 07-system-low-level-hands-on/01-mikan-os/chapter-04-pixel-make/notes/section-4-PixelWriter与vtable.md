## 4. PixelWriter、vtable 与 Placement new

---

### 一、用 C++ 类消除热路径分支

**思路：** 格式判断 **只做一次** — 实例化对应子类；循环内只调 **虚函数 Write**。

```cpp
class PixelWriter {
public:
    virtual void Write(int x, int y, const PixelColor& c) = 0;
};

class BgrPixelWriter : public PixelWriter { /* 固定 BGR 写序 */ };
class RgbPixelWriter : public PixelWriter { /* 固定 RGB 写序 */ };
```

```
启动时：if (fmt == BGR) writer = &bgr_writer; else writer = &rgb_writer;
绘图：  for … writer->Write(x, y, color);   // 无 fmt 分支
```

| 对比 | WritePixel 每像素 | PixelWriter |
|------|-------------------|-------------|
| 格式判断 | **O(像素数)** | **O(1)** |
| 循环内 | if/else | **虚调用**（固定一种实现） |

---

### 二、vtable（虚函数表）

C++ **虚函数** 底层实现：

```
对象头部 → 指向 vtable 的指针
vtable   → 函数指针数组（Write、析构等）
调用 writer->Write(...) → 通过 vtable 间接跳转
```

| 概念 | 说明 |
|------|------|
| **动态分派** | 编译期不知具体子类 → **一次间接调用** |
| **成本** | 比内联分支多一次指针跳转 — 但 **省去重复分支** |
| **对象布局** | 含虚函数的对象 **通常带 vptr** |

→ [appendix-D C++ 模板](../../appendix-D-cpp-templates/) · [CSAPP 机器级](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/)

---

### 三、Placement new（带参数的 new）

**此时尚无堆分配器** — 不能用普通 **`new`** / **`malloc`**。

**Placement new：** 在 **已预分配的内存** 上构造对象：

```cpp
alignas(PixelWriter) std::byte writer_buf[sizeof(BgrPixelWriter)];
PixelWriter* writer = new (writer_buf) BgrPixelWriter(fb, width, pitch);
// 使用 writer->Write(...)
writer->~PixelWriter();   // 手动析构 — 无全局 delete
```

| 要点 | 说明 |
|------|------|
| **`new (ptr) T(args)`** | 在 `ptr` 处调用 **T 的构造函数** |
| **不分配内存** | 内存由 **栈数组 / 静态缓冲** 提供 |
| **生命周期** | 需 **显式析构** — 直到 Ch 8 堆管理完善 |

**为何本章需要：** `BgrPixelWriter` / `RgbPixelWriter` 可能含 **帧缓冲指针等状态** — 用类封装比 C 函数指针组更清晰。

---

← [3. WritePixel](./section-3-像素格式与WritePixel.md) · 下一节 [5. ELF 加载器](./section-5-ELF格式与加载器改进.md)
