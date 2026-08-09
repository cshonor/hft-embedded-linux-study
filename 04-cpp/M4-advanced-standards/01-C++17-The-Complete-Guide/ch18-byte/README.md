# 第 18 章 std::byte

**std::byte**

## 本章讲什么

`std::byte` 是 C++17 引入的字节类型——表示"原始字节"的强类型，区别于 `char`/`unsigned char`（既是字符又是字节的双重身份）。让二进制数据处理的意图更清晰。

## 要点

### 为什么需要 byte

C++17 之前用 `char` 或 `unsigned char` 表示原始字节，但：
- `char` 有字符语义，可被 `<<` 当字符打印、`isalpha` 当字符判断。
- `char` 的符号性实现定义（signed/unsigned）。
- 用 `char*` 处理二进制数据意图不明确——是文本还是字节？

`std::byte` 是独立的枚举类型，**只有位运算**（`<<`/`>>`/`&`/`|`/`^`），没有字符语义、没有算术运算（不能 `byte + 1`）。

### 基本用法

```cpp
#include <cstddef>

std::byte b{0x42};
std::byte b2 = std::byte{0xFF};

// 位运算
b |= std::byte{0x0F};
b &= std::byte{0xF0};
b <<= 1;

// 与整数互转
int i = std::to_integer<int>(b);   // byte → int
std::byte b3 = std::byte{0x80};     // int → byte（显式）

// 不能算术
// b + 1;   // 编译错
// b++;     // 编译错
```

### 与 char/uint8_t 的对比

| 类型 | 语义 | 算术 | 位运算 | 字符操作 |
|------|------|------|--------|----------|
| `char` | 字符 | 是 | 是 | 是（`<<` 打印字符） |
| `unsigned char` | 字节/字符 | 是 | 是 | 是 |
| `uint8_t` | 8 位整数 | 是 | 是 | 是（可隐式转 int） |
| `std::byte` | 原始字节 | **否** | 是 | **否** |

### 用途

```cpp
// 二进制缓冲区
std::vector<std::byte> buf(1024);
read(socket, buf.data(), buf.size());

// 网络协议解析
void parse(const std::byte* data, size_t len) {
    uint32_t magic = std::to_integer<uint32_t>(data[0]) << 24 | ...;
}

// memcpy 仍可用
std::memcpy(buf.data(), &value, sizeof(value));
```

### 不能直接用的地方

- `std::cout << byte` 不行（无字符语义），要 `to_integer<int>`。
- `scanf`/`printf` 不直接支持 byte。
- 与 C API 互操作时仍要转 `char*`（`reinterpret_cast`）。

## HFT 关联

- **二进制协议缓冲**：行情/订单的序列化缓冲用 `vector<std::byte>`，意图清晰是原始字节不是文本。
- **网络层解析**：FIX/二进制协议解析用 `const std::byte*`，`to_integer<uint16_t>` 取字段值。
- **避免字符误操作**：`byte` 不能 `isalpha`/`<<` 当字符，防止二进制数据被误当文本处理。
- **与 `memcpy`/`mmap` 互操作**：`byte*` 和 `char*` 可 `reinterpret_cast` 互转，与 C API 兼容。
- **DPDK mbuf**：DPDK 的 `rte_mbuf` 数据区可视为 `byte*`，类型明确。

## 自测题

1. `std::byte` 相比 `char`/`unsigned char` 有什么语义优势？
2. `byte` 支持哪些运算？不支持哪些？
3. `byte` 和整数如何互转？
4. `std::cout << byte` 为什么不行？怎么打印？
5. HFT 二进制协议缓冲为什么用 `vector<std::byte>` 而非 `vector<char>`？

## 代码自测

### Q1: std::byte
```cpp
// C++17 前：用 char/unsigned char 表示原始字节
unsigned char buf[1024];

// C++17: std::byte 语义明确
std::byte buf2[1024];

// 运算
std::byte b{0xFF};
b |= std::byte{0x0F};     // 位运算
b <<= 1;                  // 移位
int val = std::to_integer<int>(b);  // 转整数
```
> std::byte 相比 unsigned char 有什么好处？为什么不直接用 int？

<details>
<summary>答案与复习指引</summary>

**好处**：
1. **语义明确**：`std::byte` 表示"原始字节，不是字符也不是数字"，避免误用算术运算
2. **类型安全**：`std::byte` 只支持位运算（`&`/`|`/`^`/`<<`/`>>`），不支持 `+`/`-`/`*`，防止意外算术
3. **意图清晰**：API 用 `std::byte*` 明确表示"处理原始内存"

**为什么不用 int**：int 有符号、大小不固定（4 或 8 字节）、支持算术运算——处理二进制数据时不安全。

**复习：** → [std::byte](./README.md)
</details>
