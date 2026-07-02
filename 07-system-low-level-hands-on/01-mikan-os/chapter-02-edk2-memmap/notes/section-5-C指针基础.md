## 5. C/C++ 指针基础

> 本章 **GetMemoryMap**、文件协议调用大量用到指针 — 末尾专章补齐基础。

---

### 一、指针是什么

**指针 = 存放「变量地址」的变量。**

```c
UINT32 value = 42;
UINT32 *ptr = &value;   // ptr 保存 value 的地址

*ptr = 100;             // 通过指针修改 value → 100
```

| 运算符 | 含义 |
|--------|------|
| **`&x`** | 取 `x` 的地址 |
| **`*p`** | 解引用 — 访问 `p` 指向的对象 |
| **类型 `T*`** | 指向 `T` 的指针 |

→ [CSAPP Ch3 程序结构与指针](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/)

---

### 二、箭头运算符 `->`

当指针指向 **结构体** 时，访问成员有两种写法：

```c
EFI_SYSTEM_TABLE *SystemTable = ...;

(*SystemTable).ConOut->OutputString(...);  // 合法但繁琐
SystemTable->ConOut->OutputString(...);    // 常用：ptr->member
```

| 形式 | 等价 |
|------|------|
| **`ptr->field`** | **`(*ptr).field`** |

**UEFI 代码风格：** 几乎总是 **`SystemTable->BootServices->GetMemoryMap(...)`** 链式调用。

---

### 三、指针的指针（`**`）

UEFI 接口中极常见 — 函数需要 **修改调用者的指针变量**（输出参数）：

```c
EFI_STATUS GetMemoryMap(
    IN OUT UINTN              *MemoryMapSize,
    OUT    EFI_MEMORY_DESCRIPTOR *MemoryMap,
    ...
);
```

| 模式 | 原因 |
|------|------|
| **`OUT void **`** | 分配后把 **新缓冲区地址** 写回给调用者 |
| **`IN OUT UINTN *`** | 传入缓冲大小，返回 **实际所需/已用大小** |
| **协议 `OpenProtocol`** | 输出 **`EFI_FILE_PROTOCOL **`** — 得到文件接口指针 |

**读法：** 参数类型里的 `*` 层数 = 「传地址以便被 callee 改写」。

```
调用者:  EFI_FILE_PROTOCOL *File;
         Open(..., &File);     // &File 是 EFI_FILE_PROTOCOL**

被调者:  写入 *File = 新指针;
```

---

### 四、与 UEFI 协议的关系

UEFI 设计为 **C 接口 + 函数表（vtable 风格）**：

```c
File->Write(File, &BufferSize, Buffer);
//  ↑ 协议实例   ↑ 方法指针
```

| 概念 | 对应 |
|------|------|
| **`EFI_FILE_PROTOCOL *File`** | 对象 |
| **`File->Write`** | 方法（函数指针） |
| **第一个参数 `File`** | 多数协议函数的 **this** 指针 |

理解 **`->` 与 `**`** 后，阅读 EDK II 源码与后续 **PCI、Graphics** 协议会顺畅很多。

---

← [4. GetMemoryMap](./section-4-GetMemoryMap与导出memmap.md) · 下一节 [6. 小结](./section-6-小结与索引.md)
