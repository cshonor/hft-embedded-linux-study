# 18.1 std::byte

> 第 18 章 std::byte

## 这节讲什么

`std::byte` 是 C++17 引入的类型安全字节类型。C/C++ 之前用 `char`/`unsigned char` 表示"原始字节"，但 `char` 既可以是字符也可以是字节，语义模糊。`std::byte` 明确表示"这就是一个字节，不是字符"。

## 为什么要这个

```cpp
// C：char 既是字符也是字节
char buf[1024];       // 是字符数组还是字节缓冲区？
unsigned char* p = (unsigned char*)&data;  // 类型转换

// C++17：std::byte 明确表示字节
std::byte buf[1024];  // 明确：这是字节缓冲区
std::byte* p = reinterpret_cast<std::byte*>(&data);
```

## 基本用法

```cpp
// 创建
std::byte b1{42};
std::byte b2 = std::byte{0xFF};

// 位运算（byte 只支持位运算，不支持算术）
b1 | b2;
b1 & b2;
b1 ^ b2;
~b1;
b1 << 2;
b1 >> 2;

// 转换为整数
int x = std::to_integer<int>(b1);
unsigned char c = std::to_integer<unsigned char>(b2);

// 从整数构造
std::byte b3{0xAB};
// std::byte b4 = 0xAB;  // 编译错误：不允许隐式转换
```

## byte vs unsigned char

| 特性 | `unsigned char` | `std::byte` |
|------|----------------|-------------|
| 算术运算 | `a + b` ✅ | ❌（只有位运算） |
| 字符语义 | 有（`cout << c`） | 无 |
| 隐式转换 | `int x = c` ✅ | ❌（必须 `to_integer`） |
| 意图 | 模糊 | 明确：字节 |

## HFT 关联

```cpp
// 网络缓冲区
struct Packet {
    std::byte header[16];
    std::byte payload[1024];
};

// 序列化
void serialize(std::byte* buf, const Order& ord) {
    std::memcpy(buf, &ord, sizeof(Order));
}

// 反序列化
Order deserialize(const std::byte* buf) {
    Order ord;
    std::memcpy(&ord, buf, sizeof(Order));
    return ord;
}

// 字节检查
bool is_valid_header(const std::byte* buf) {
    return std::to_integer<uint8_t>(buf[0]) == 0x01;
}
```

## 小结

`std::byte` 是语义明确的字节类型，只支持位运算，不做算术。HFT 中用 `std::byte` 替代 `unsigned char` 做原始缓冲区，意图更清晰。

---

← [本章导读](./README.md)
