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
