# TLPI 第 41 章 — Fundamentals of Shared Libraries

**优先级**：🔴（部署、插件铺垫、ABI 版本）  
**前置**：[Ch40 登录记账](../chapter-40-login-accounting/README.md)  
**后置**：[Ch42 共享库高级 / dlopen](../chapter-42-shared-libraries-advanced/README.md)

---

## 小节目录

- [41.1 –41.3 静态 vs 共享](notes/41.1-object-libraries.md)
- [41.4 构建](notes/41.4-creating-and-using-shared-libraries-a-fi.md)
- [41.5 工具](notes/41.5-useful-tools-for-working-with-shared-lib.md)
- [41.6 –41.9 版本与安装](notes/41.6-shared-library-versions-and-naming-conve.md)
- [41.10 –41.11 RPATH / RUNPATH · 搜索顺序](notes/41.10-specifying-library-search-directories-in.md)
- [41.12 符号解析 · Interposition](notes/41.12-run-time-symbol-resolution.md)

---

## 章节目标


`.a` vs `.so`；PIC；soname 三名；构建安装/`ldconfig`；RPATH/RUNPATH；搜索顺序；符号介入与绑定；工具链。

---


---

## 易错清单


1. 忘 `-fPIC`  
2. 搞混 real/soname/linker name  
3. 装库不 `ldconfig`  
4. SUID + 依赖 `LD_LIBRARY_PATH`  
5. RPATH vs RUNPATH  
6. 未预期的符号介入  

---


---

## 实验清单


1. 带 soname 的三链构建  
2. RUNPATH vs `LD_LIBRARY_PATH`  
3. （选）interposition / `-Bsymbolic`  
4. `LD_BIND_NOW`  
5. `$ORIGIN`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | `.so` 要 PIC；运行找 soname |
| 2 | real / soname / linker 三名 |
| 3 | 新项目用 RUNPATH + `$ORIGIN` |
| 4 | 搜索序：RPATH→LDLP→RUNPATH→cache→默认 |
| 5 | SUID 忽略 LD_LIBRARY_PATH |
| 6 | 默认可符号介入；lazy PLT |

---


---

## 参考


- Kerrisk · TLPI Ch41  
- `man 8 ldconfig` · `man 1 ldd` · `man 1 ld.so`


---

## 代码示例

```c
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

/* Ch41 共享库 — dlopen/dlsym/dlclose 动态加载。
 * 演示运行时加载 libm 并调用 sqrt。
 * 编译: gcc -o ch41_demo ch41_demo.c -ldl */

int main(void) {
    /* 动态加载数学库 */
    void *handle = dlopen("libm.so.6", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    /* 查找 sqrt 函数符号 */
    dlerror();  /* 清除错误 */
    double (*sqrt_fn)(double) = dlsym(handle, "sqrt");
    const char *err = dlerror();
    if (err) {
        fprintf(stderr, "dlsym failed: %s\n", err);
        dlclose(handle);
        return 1;
    }

    /* 调用动态加载的函数 */
    double val = 144.0;
    double result = sqrt_fn(val);
    printf("sqrt(%.0f) = %.4f\n", val, result);

    /* 查找 pow 函数 */
    double (*pow_fn)(double, double) = dlsym(handle, "pow");
    if (pow_fn) {
        printf("pow(2.0, 10.0) = %.4f\n", pow_fn(2.0, 10.0));
    }

    /* 列出链接的共享库 (通过 /proc/self/maps) */
    printf("\nLinked libraries (see /proc/self/maps):\n");
    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, ".so") && !strstr(line, "["))
                printf("  %s", line);
        }
        fclose(fp);
    }

    dlclose(handle);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
