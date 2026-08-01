## 5. ELF 格式与命令行参数

---

### 一、为何需要 ELF

**裸 .bin** 无法表达：

| 需求 | ELF 提供 |
|------|----------|
| **入口点** | `e_entry` — 非文件 offset 0 |
| **C/C++ 编译产物** | `.text/.data/.bss` 段 |
| **与内核加载对照** | Ch3 **kernel.elf** 同源格式 |

---

### 二、识别与解析

```cpp
if (buf[0]==0x7f && buf[1]=='E' && buf[2]=='L' && buf[3]=='F') {
    // 读 Elf64_Ehdr
    entry = ehdr.e_entry;   // 或按 PT_LOAD 映射后重定位
    // 本章简化：加载到固定基址 + 使用文件内 entry
}
```

| 步骤 | 说明 |
|------|------|
| **魔数** | `\x7f ELF` |
| **e_entry** | 跳 **main 前 crt 或 main 本身**（视链接脚本） |
| **Program Header** | **PT_LOAD** — 拷贝 **可加载段** 到内存 |

→ [Ch3 内核 ELF](../chapter-03-bootloader-display/notes/section-3-第一个内核与ELF加载.md)

---

### 三、MakeArgVector()

**终端一行 → C 主函数参数：**

```
> rpn 2 3 +
  cmd=rpn  argv=["rpn","2","3","+"]
```

```cpp
int argc;
char** argv = MakeArgVector(linebuf_, argc);
using MainFn = int(*)(int, char**);
int exit_code = main_fn(argc, argv);
Print("exit code: %d\n", exit_code);
```

| 细节 | 说明 |
|------|------|
| **空格分词** | 与 Ch16 **echo** 同源逻辑 · 可复用 |
| **argv[0]** | 传统上 **程序名** |
| **退出码** | **返回值** 打印到终端 — **shell 语义雏形** |

---

### 四、裸 bin vs ELF 分支

```
读文件完成
    ↓
ELF? → 解析 entry + 段加载 → call(argc, argv)
否则 → jmp load_base
```

---

← [4. sti](./section-4-sti与hlt冻结Bug.md) · 下一节 [6. C++/libc++](./section-6-C++应用标准库与小结.md)
