## 6. readfile、grep 与小结

---

### 一、readfile 应用

```cpp
#include <stdio.h>

int main(int argc, char** argv) {
    FILE* fp = fopen(argv[1], "r");
    char line[256];
    for (int i = 0; i < 3 && fgets(line, sizeof line, fp); ++i)
        printf("%s", line);
    fclose(fp);
    return 0;
}
```

```
> readfile readme.txt
(打印前三行)
```

| 验证 | 说明 |
|------|------|
| **fopen/fgets** | 全链 **open/read** |
| **路径参数** | **FindFile 递归** |
| **stdio 缓冲** | Newlib **用户态缓冲** + **syscall read** |

---

### 二、grep 与 `<regex>`

```cpp
#include <regex>
#include <fstream>
#include <string>

// 对文件逐行 regex_search
std::regex re(pattern);
while (std::getline(in, line)) {
    if (std::regex_search(line, match, re))
        printf("%s\n", line.c_str());
}
```

| 依赖 | 补充 |
|------|------|
| **libc++ `<regex>`** | 内部 **堆分配** |
| **posix_memalign()** | **对齐分配** — 在 **newlib_support** 实现 |

**能力跃迁：** 应用 **文本处理管道** — 接近 **Unix 工具链** 雏形。

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **FindFile 递归 · ls 路径** | **目录树** |
| **apps/** | 构建 **APPS_DIR** |
| **fd · FileDescriptor** | **跨簇 read** |
| **OpenFile/ReadFile** | **内核 syscall** |
| **open/read/sbrk** | **Newlib 桥** |
| **readfile · grep** | **stdio · regex** |

```
Ch25 读文件
    ↓
Ch26 写文件
Ch29 IPC
```

---

### 四、后续索引

| Ch25 主题 | 继续读 |
|----------|--------|
| 写文件 | [chapter-26-app-write-file](../chapter-26-app-write-file/) 🟡 |
| FAT 基础 | [chapter-17-filesystem](../chapter-17-filesystem/) |
| syscall | [chapter-20-syscall](../chapter-20-syscall/) 🔴 |
| TLPI | [08-TLPI](../../../08-The-Linux-Programming-Interface/) |

---

← [5. Newlib](./section-5-OpenFile-ReadFile与Newlib.md) · [Ch 24](../chapter-24-multi-terminal/) · [Ch 25 导读](../README.md)
