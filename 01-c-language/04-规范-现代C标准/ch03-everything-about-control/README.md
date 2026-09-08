# Ch3 · Everything is about control（一切都与控制有关）

> **Level 1 · 相识** · 策略：**⏭️ 跳过**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 本章讲什么

`if`/`switch`/`for`/`while`/`break`/`continue`/`goto`。K&R Ch3 已完整覆盖控制流基础；
本章只关注 **C23 新增的控制流变化**。

## 一、C23 新增：条件中的声明（init-statement）

类似 C++17 的 `if (init; condition)`：

```c
/* C23：if 带初始化语句 */
if (struct packet *p = recv(); p != NULL) {
    process(p);
    free(p);
}
/* p 的作用域到 if-else 结束 */

/* C23：switch 带初始化语句 */
switch (int c = getc(stdin); c) {
    case 'q': return;
    default:  printf("%c", c);
}
```

| 好处 | 说明 |
|------|------|
| 缩小变量作用域 | 声明只在 `if` 块内有效，避免污染外部作用域 |
| 避免 "使用前忘记检查" | 声明和条件绑在一起，编译器会警告未检查 |

## 二、C23 新增：`[[fallthrough]]` 属性

`switch` 的 fall-through 是经典 bug 源。C23 引入显式标注：

```c
switch (cmd) {
    case PARSE_HEADER:
        parse_header(pkt);
        [[fallthrough]];      /* 告诉编译器：故意 fall-through */
    case PARSE_BODY:
        parse_body(pkt);
        break;
    default:
        break;
}
```

> gcc/clang 的 `-Wimplicit-fallthrough` 会警告未标注的 fall-through；`[[fallthrough]]` 是标准化的静默方式。

## 三、C23 新增：`[[nodiscard]]` 属性

```c
[[nodiscard]] int check_checksum(const void *buf, size_t len);

/* 调用时如果忽略返回值，编译器警告 */
check_checksum(pkt, 64);    /* ⚠ warning: ignoring return value */
```

HFT 场景极有用：校验函数的返回值不能忽略，否则可能处理了坏数据包。

## 四、goto：错误处理的正确用法

K&R 已讲 `goto`，Modern C 给出系统化建议。**内核和 DPDK 的错误处理路径大量使用 `goto`**：

```c
int init_pipeline(void)
{
    int *a = malloc(100 * sizeof(int));
    if (!a) goto fail_a;

    int *b = malloc(200 * sizeof(int));
    if (!b) goto fail_b;

    int *c = malloc(300 * sizeof(int));
    if (!c) goto fail_c;

    /* 成功路径 */
    return 0;

fail_c:
    free(b);
fail_b:
    free(a);
fail_a:
    return -1;
}
```

| 规则 | 说明 |
|------|------|
| `goto` 只往前跳 | 跳到清理标签，不回跳（避免变成循环） |
| 清理标签逆序排列 | 先分配的最后释放（LIFO 栈式） |
| 不要用 `goto` 代替循环 | 循环用 `for`/`while`，`goto` 只用于错误清理 |

> 这正是内核 `goto err` 模式，见 LKD。

## HFT / DPDK 关联

- `[[fallthrough]]` 在协议解析状态机中有用（HDR → BODY → CHECKSUM 连续处理）
- `goto cleanup` 是 DPDK 初始化代码的标准模式（`rte_eal_init` 失败路径）
- C23 init-statement 在 `for` 循环中早已是 C99 的 `for (int i = 0; ...)`，`if`/`switch` 版是新增量

## 自测题

<details><summary>1. <code>[[fallthrough]]</code> 解决了什么问题？</summary>

`switch` case 不写 `break` 会 fall-through 到下一个 case，这是经典 bug 源。
`[[fallthrough]]` 显式标注"故意 fall-through"，让编译器的 `-Wimplicit-fallthrough` 警告静默，
同时向读者传达意图。C23 之前用 `/* fallthrough */` 注释，编译器识别但不标准化。
</details>

<details><summary>2. 为什么错误清理用 <code>goto</code> 而不是多层 <code>if-else</code>？</summary>

多层 `if-else` 会导致"箭头代码"（嵌套越来越深），可读性差。`goto cleanup` 保持正常路径在最外层，
清理路径集中管理，更接近内核/DPDK 的惯用风格。关键是 `goto` 只往前跳到清理标签，不回跳。
</details>
