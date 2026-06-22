## 7.9–7.11 加载与动态链接

### 7.9 加载可执行目标文件

- `execve` 内核 **创建进程地址空间**，映射 **PT_LOAD** 段
- **`.bss`** 分配零页；**栈、堆** 随后增长
- 细节 → [Ch 9 虚拟内存](../chapter-09-virtual-memory/)

### 7.10 动态链接共享库

- **`.so`** — 运行时 **共享** 一份物理代码（TEXT 共享、DATA 每进程副本）
- **节省磁盘与 RAM**；**升级 libc** 不必重编所有程序
- 链接器 `ld.so` / `ld-linux-x86-64.so.2` **重定位共享库** 并绑定符号

**延迟绑定 (lazy binding)：**  
`PLT` (Procedure Linkage Table) + `GOT` (Global Offset Table) — 首次调用跳 `ld.so` 解析，之后走缓存地址。

```bash
ldd ./prog
LD_DEBUG=bindings ./prog   # 调试加载
```

### 7.11 从应用程序加载共享库

```c
#include <dlfcn.h>
void *handle = dlopen("libstrategy.so", RTLD_LAZY);
void (*fn)(void) = dlsym(handle, "on_tick");
fn();
dlclose(handle);
```

**HFT：**

| 方式 | 场景 |
|------|------|
| 编译期 `-lfoo` | 固定依赖 |
| **`dlopen` 插件** | 策略 .so 热换（注意 **ABI、RTLD、线程安全**） |
| **`LD_PRELOAD`** | 打桩/调试（生产慎用） |

**启动延迟：** 动态链接 + 大量 `.so` → **relocate 与 page fault**；低延迟服务常 **减少依赖** 或 **preload 映射**。

---

← [本章导读](../README.md)
