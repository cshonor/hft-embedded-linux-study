# Ch41 demos — Shared library basics

在 Linux / WSL 下于本目录执行：

```bash
# 1) build shared lib with soname
gcc -Wall -Wextra -fPIC -c foo.c
gcc -shared -Wl,-soname,libfoo.so.1 -o libfoo.so.1.0.0 foo.o
ln -sf libfoo.so.1.0.0 libfoo.so.1
ln -sf libfoo.so.1 libfoo.so

# 2) link app with RUNPATH = $ORIGIN (find .so next to binary)
gcc -Wall -Wextra -o app main.c -L. -lfoo \
  -Wl,--enable-new-dtags,-rpath,'$ORIGIN'

./app
readelf -d app | grep -E 'NEEDED|RUNPATH|RPATH'
ldd ./app

# optional: force bind-now
LD_BIND_NOW=1 ./app
```

| 文件 | 说明 |
|------|------|
| `foo.h` / `foo.c` | 迷你共享库 |
| `main.c` | 链接 `-lfoo` 的程序 |

## 代码示例

```c
#include <stdio.h>
#include <dlfcn.h>
/* Ch41 demo: dlopen/dlsym (need -ldl) */
int main(void) {
    void *h = dlopen("libm.so.6", RTLD_LAZY);
    double (*sq)(double) = dlsym(h, "sqrt");
    printf("sqrt(2)=%f\n", sq(2.0));
    dlclose(h);
    return 0;
}
```

---
