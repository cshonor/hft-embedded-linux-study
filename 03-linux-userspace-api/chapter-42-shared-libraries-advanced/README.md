# TLPI 第 42 章 — Advanced Features of Shared Libraries

**优先级**：🔴（插件架构、符号可见性）  
**前置**：[Ch41 共享库基础](../chapter-41-shared-libraries/notes.md)  
**后置**：[Ch43 IPC 综述](../chapter-43-ipc-overview/notes.md)  

---

## 小节目录

- [42.1 –42.4 dlopen 家族（`<dlfcn.h>`，`-ldl`）](./notes/42.1-dlopen-dlfcn-ldl.md)
- [42.5 dladdr](./notes/42.5-dladdr.md)
- [42.6 初始化 / 终止](./notes/42.6-initialization-termination.md)
- [42.7 符号可见性](./notes/42.7-symbol.md)
- [42.8 –42.9 命名空间与环境变量](./notes/42.8-environment-namespace.md)

---

## 章节目标


运行时动态加载；`RTLD_*` 绑定/作用域；`dlerror`/`dladdr`；constructor/destructor；符号可见性；`dlclose` 陷阱；相关环境变量；稳定插件接口。

---


---

## 易错清单


1. 忘 `-ldl`  
2. C++ 无 `extern "C"`  
3. `dlclose` 后仍调插件  
4. 插件互依赖却全用 `RTLD_LOCAL`  
5. 不查 `dlerror`  
6. 生产逻辑绑死 `LD_PRELOAD`  

---


---

## 实验清单


1. `dlopen` + `dlsym` 调插件  
2. `RTLD_LAZY` vs `RTLD_NOW`  
3. constructor/destructor 观察  
4. `dladdr` 打印符号  
5. （选）`RTLD_NODELETE` / visibility  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | `-ldl`；`dlerror` 读一次清一次 |
| 2 | `NOW`/`LAZY` × `GLOBAL`/`LOCAL` |
| 3 | 插件：`NOW\|LOCAL` + 接口结构体 |
| 4 | `dlclose` 后野指针；`NODELETE` 可留库 |
| 5 | constructor/destructor 优于 `_init` |
| 6 | `-fvisibility=hidden` + 显式 default 导出 |

---


---

## 参考


- Kerrisk · TLPI Ch42  
- `man 3 dlopen` · `man 3 dladdr` · `man 8 ld.so`


---

## 代码示例

```c
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <link.h>

/* Ch42 共享库高级 — dlopen 标志/符号版本/初始化/析构。
 * 演示 RTLD_GLOBAL vs RTLD_LOCAL + 构造/析构函数。
 * 编译: gcc -o ch42_demo ch42_demo.c -ldl */

/* 共享库的构造和析构函数 */
__attribute__((constructor))
static void init_func(void) {
    printf("[constructor] Library loaded\n");
}

__attribute__((destructor))
static void fini_func(void) {
    printf("[destructor] Library unloaded\n");
}

/* 遍历已加载的共享库 */
static int callback(struct dl_phdr_info *info, size_t size, void *data) {
    static int count = 0;
    if (info->dlpi_name && info->dlpi_name[0])
        printf("  [%d] %s\n", count++, info->dlpi_name);
    return 0;
}

int main(void) {
    /* RTLD_NOW: 立即解析所有符号 (vs RTLD_LAZY 延迟) */
    void *h1 = dlopen("libm.so.6", RTLD_NOW | RTLD_LOCAL);
    if (h1) {
        printf("Loaded libm.so.6 with RTLD_NOW|RTLD_LOCAL\n");

        /* 深绑定: RTLD_DEEPBIND 优先搜索自己 */
        void *h2 = dlopen("libc.so.6", RTLD_NOW | RTLD_GLOBAL);
        if (h2) {
            printf("Loaded libc.so.6 with RTLD_GLOBAL (symbols available to later libs)\n");
            dlclose(h2);
        }
        dlclose(h1);
    }

    /* 遍历已加载库 */
    printf("\nLoaded shared objects:\n");
    dl_iterate_phdr(callback, NULL);

    /* RPATH/RUNPATH: 编译时指定搜索路径
     * gcc -Wl,-rpath,/custom/lib -o demo demo.c
     * ldd demo 查看依赖
     * LD_LIBRARY_PATH 也可以设置运行时搜索路径
     */
    printf("\nRuntime search paths:\n");
    printf("  LD_LIBRARY_PATH: %s\n", getenv("LD_LIBRARY_PATH") ?: "(not set)");
    printf("  Use 'ldd <binary>' to see dependencies\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
