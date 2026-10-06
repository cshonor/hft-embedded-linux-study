# ch02 Demo

```bash
make -C demo01_sizeof && ./demo01_sizeof/main
make -C demo02_char_promotion && ./demo02_char_promotion/main
```

## demo01_sizeof

各基础类型 / `stdint` 定宽类型的宽度（[2.2](../2.2-数据类型及长度.md)、[2.2.3](../2.2.3-stdint.h.md)）。

## demo02_char_promotion

字符常量的类型与整数提升的边界（[2.3.1](../2.3.1-字符常量的类型与整型提升.md)）：

1. `sizeof('c')` = 4（字符常量）vs `sizeof(c)` = 1（`char` 变量）
2. `_Generic` 类型取证：`'c'` → `int`，`c` → `char`
3. `sizeof` 的操作数 **不做** 整数提升（`sizeof(+c)` = 4）
4. 提升的正收益：`unsigned char` 小减大得负数而非回绕
5. `char ch = (char)0xFF` 经典坑 + 三种修法
6. `printf` 可变参数：提升由编译器自动完成

### 双平台实测（两边都零告警）

| | Mac (arm64) | Raspberry Pi 5 (aarch64) |
|---|---|---|
| 编译器 | clang 23.1.0（`arm64-apple-darwin`） | gcc 14.2.0（`aarch64-linux-gnu`） |
| `sizeof('c')` | 4 | 4 |
| `char` 符号性 | **signed** | **unsigned** |
| `(int)(char)0xFF` | -1 | 255 |
| `ch == 0xFF` | false | **true** |
| `-Wall -Wextra` 下的 `ch == -1` | 无告警 | 无告警（改用变量后） |

Pi 上的构建与运行：

```bash
# Mac → Pi（rsync 或 scp 整个 ch02 目录）后：
make -C demo && ./demo/demo02_char_promotion/main
```

**GCC 告警文本**（需 `char` 为 signed 才触发，故用 `-fsigned-char` 复现）：

```
warning: comparison is always false due to limited range of data type [-Wtype-limits]
```

`-Wtype-limits` 已含在 `-Wextra` 里；clang 侧叫 `-Wtautological-constant-out-of-range-compare`（GCC 不认识这个名字）。

**C++ 对照** —— 同一个 `sizeof('c')`，C++ 得到 **1** 而不是 4（普通字符字面量在 C++ 里是 `char`），g++ 14.2 与 clang++ 23.1.0 均如此：

```bash
clang++ -std=c++17 -x c++ -o /tmp/lit - <<'EOF' && /tmp/lit
#include <cstdio>
int main() { std::printf("%zu\n", sizeof('c')); }
EOF
# 输出：1
```
