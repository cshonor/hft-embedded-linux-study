# Ch42 demos — dlopen plugin

在 Linux / WSL 下于本目录执行：

```bash
# plugin (.so) with constructor/destructor + visibility
gcc -Wall -Wextra -fPIC -fvisibility=hidden -c plugin.c
gcc -shared -o plugin.so plugin.o

# host: must link -ldl
gcc -Wall -Wextra -o host host.c -ldl

./host ./plugin.so

# optional debug
LD_DEBUG=libs ./host ./plugin.so 2>&1 | head
```

| 文件 | 说明 |
|------|------|
| `plugin_api.h` | 插件接口结构体 |
| `plugin.c` | 导出 `plugin_get_api`；ctor/dtor |
| `host.c` | `dlopen`/`dlsym`/`dladdr`/`dlclose` |

## 代码示例

```c
#include <stdio.h>
#include <dlfcn.h>
/* Ch42 demo: dlopen RTLD_NOW vs RTLD_LAZY (need -ldl) */
int main(void) {
    void *h = dlopen("libm.so.6", RTLD_NOW);
    printf("loaded with RTLD_NOW: %s\n", h ? "OK" : dlerror());
    if (h) dlclose(h);
    return 0;
}
```

---
