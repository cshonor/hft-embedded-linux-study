# 函数指针 typedef 通用模板

**Expert C ch10** — 中断 / 回调 / 命令表 / 驱动注册表。

## 1. 基础回调（无参 / 单参）

```c
typedef void (*Callback)(void);
typedef void (*CallbackInt)(int code);

void register_cb(Callback cb) { if (cb) cb(); }
```

## 2. 带上下文（内核/驱动常用）

```c
typedef int (*IrqHandler)(void *dev_id);

struct device {
    void *priv;
    IrqHandler handler;
};

static int my_irq(void *dev_id)
{
    (void)dev_id;
    return 0; /* handled */
}
```

## 3. 命令表 / 分发器

```c
typedef int (*CmdFunc)(int argc, char **argv);

struct cmd_entry {
    const char *name;
    CmdFunc     fn;
};

static int cmd_help(int argc, char **argv);
static int cmd_quit(int argc, char **argv);

static struct cmd_entry commands[] = {
    { "help", cmd_help },
    { "quit", cmd_quit },
    { NULL,   NULL      },
};

static int dispatch(const char *name, int argc, char **argv)
{
    for (struct cmd_entry *e = commands; e->name; e++)
        if (e->fn && name && e->name[0] == name[0]) /* 简化匹配 */
            return e->fn(argc, argv);
    return -1;
}
```

## 4. 同签名函数指针数组（中断向量风格）

```c
typedef void (*IsrFunc)(int irq);

static void isr_timer(int irq) { (void)irq; }
static void isr_uart(int irq)  { (void)irq; }

static IsrFunc vector[32];

static void init_vector(void)
{
    for (int i = 0; i < 32; i++)
        vector[i] = NULL;
    vector[0] = isr_timer;
    vector[1] = isr_uart;
}

static void run_isr(int irq)
{
    if (irq >= 0 && irq < 32 && vector[irq])
        vector[irq](irq);
}
```

## 5. signal 原型（返回函数指针的函数）

```c
/* void (*signal(int sig, void (*handler)(int)))(int); */
typedef void (*SigHandler)(int);
SigHandler signal(int sig, SigHandler handler); /* 简化 typedef 后 */
```

## 6. 工程规范

| 规则 | 说明 |
|------|------|
| **typedef 命名** | `XxxHandler` / `XxxCallback` / `XxxFunc` |
| **NULL 检查** | 调用前 `if (cb) cb(arg);` |
| **const 表** | 命令表用 `static const struct cmd_entry` |
| **避免 cast** | 尽量不用 `(void*)` 强转函数指针 |

## Demo

见 [demo/demo02_func_ptr](./demo/demo02_func_ptr/main.c)。
