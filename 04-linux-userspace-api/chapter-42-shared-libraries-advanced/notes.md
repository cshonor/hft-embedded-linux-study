# TLPI 第 42 章 — Advanced Features of Shared Libraries

> 对应目录：`chapter-42-shared-libraries-advanced/`  
> 书名原文：**Advanced Features of Shared Libraries**  
> ⚠️ **插件用 `dlopen`/`dlsym`，链接加 `-ldl`。** 推荐 `RTLD_NOW | RTLD_LOCAL` + 显式接口结构体；`dlclose` 后勿再调插件代码（或用 `RTLD_NODELETE`）。C++ 导出必须 `extern "C"`。

**优先级**：🔴（插件架构、符号可见性）  
**前置**：[Ch41 共享库基础](../chapter-41-shared-libraries/notes.md)  
（勿用 `…-shared-libraries-basics`）  
**后置**：[Ch43 IPC 综述](../chapter-43-ipc-overview/notes.md)  
（地图路径是 `chapter-43-ipc-overview`，非 `…-interprocess-communication-intro`）

---

## 章节目标

运行时动态加载；`RTLD_*` 绑定/作用域；`dlerror`/`dladdr`；constructor/destructor；符号可见性；`dlclose` 陷阱；相关环境变量；稳定插件接口。

---

## 42.1–42.4 dlopen 家族（`<dlfcn.h>`，`-ldl`）

```c
void *dlopen(const char *filename, int flags);
void *dlsym(void *handle, const char *symbol);
int   dlclose(void *handle);
char *dlerror(void);   /* 读一次即清零 */
```

### Flags

| 组 | 标志 | 含义 |
|----|------|------|
| 绑定 | `RTLD_LAZY` | 首次调用再解析（默认） |
| | `RTLD_NOW` | 打开时解析完；缺符号立即失败 |
| 作用域 | `RTLD_LOCAL` | 符号不进全局空间（默认） |
| | `RTLD_GLOBAL` | 后续库可解析本库符号 |
| 附加 | `RTLD_NODELETE` | `dlclose` 后仍不卸载映射 |

```c
dlopen("./plugin.so", RTLD_NOW | RTLD_LOCAL);
```

插件实践：`RTLD_NOW | RTLD_LOCAL` + 通过 `dlsym` 取**一个** `plugin_api` 结构体指针，避免库间互相 `dlsym` 依赖。

### dlsym

返回 `void*`；转函数指针宜用 typedef + 中转赋值。C++ 需 `extern "C"` 防 name mangling。

### dlerror

每次调用会清除状态。模板：先 `dlerror()` 清空 → `dlsym` → 再 `dlerror()` 判错（区分「符号不存在」与「合法 NULL」）。

### dlclose

引用计数到 0 → 跑 destructor → 默认卸载。  
陷阱：插件线程/回调仍存活 → SIGSEGV。先注销回调/停线程，或 `RTLD_NODELETE`。

Demo：[`code/`](./code/)

---

## 42.5 dladdr

```c
typedef struct {
    const char *dli_fname;
    void       *dli_fbase;
    const char *dli_sname;
    void       *dli_saddr;
} Dl_info;
```

地址 → 所属库/符号；用于日志、简易栈信息。

---

## 42.6 初始化 / 终止

| 方式 | 说明 |
|------|------|
| `_init` / `_fini` | 老式，不推荐 |
| `__attribute__((constructor))` / `destructor` | 现代推荐；可带优先级 `(100)` |

加载时 constructor（优先级升序）；卸载时 destructor（反向）。

---

## 42.7 符号可见性

```bash
gcc -fvisibility=hidden -fPIC -c ...
```

```c
__attribute__((visibility("default"))) void exported_api(void);
```

缩小导出表、减轻符号冲突/介入、利于内部优化。

---

## 42.8–42.9 命名空间与环境变量

Linker namespaces：Linux 扩展，隔离多套全局符号空间（了解即可）。

| 变量 | 作用 |
|------|------|
| `LD_LIBRARY_PATH` | 搜索路径（SUID 忽略） |
| `LD_BIND_NOW` | 强制立即绑定 |
| `LD_PRELOAD` | 预加载劫持（SUID 忽略） |
| `LD_DEBUG=libs` / `all` | 调试动态链接 |

---

## 易错清单

1. 忘 `-ldl`  
2. C++ 无 `extern "C"`  
3. `dlclose` 后仍调插件  
4. 插件互依赖却全用 `RTLD_LOCAL`  
5. 不查 `dlerror`  
6. 生产逻辑绑死 `LD_PRELOAD`  

---

## 实验清单

1. `dlopen` + `dlsym` 调插件  
2. `RTLD_LAZY` vs `RTLD_NOW`  
3. constructor/destructor 观察  
4. `dladdr` 打印符号  
5. （选）`RTLD_NODELETE` / visibility  

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

## 参考

- Kerrisk · TLPI Ch42  
- `man 3 dlopen` · `man 3 dladdr` · `man 8 ld.so`
