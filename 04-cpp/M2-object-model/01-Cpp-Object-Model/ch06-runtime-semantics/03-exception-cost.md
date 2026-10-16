# 6.3 异常处理开销

> 第 6 章 · 上一节：[6.2 RTTI](02-rtti.md) · 下一章：[第 7 章 高级对象模型](../ch07-advanced-object-model/README.md)

## 这节讲什么

C++ 的零开销异常模型——正常路径几乎零开销（异常表在 .gcc_except_table），抛异常极慢（栈展开 + 查表）。`-fno-exceptions` 关闭异常可减小二进制。

---

## 为什么要学这个（先建立直觉）

C 程序员用错误码，没有异常的概念：

```c
// C：错误码
int read_data(int fd, char* buf, int len) {
    int n = read(fd, buf, len);
    if (n < 0) return -1;  // 错误码
    return n;
}
// 正常路径和错误路径代价相同（都是 if 判断）
```

C++ 的异常是"零开销模型"——正常路径几乎免费，但抛异常极慢：

```cpp
// C++：异常
int read_data(int fd, char* buf, int len) {
    int n = read(fd, buf, len);
    if (n < 0) throw std::runtime_error("read failed");
    return n;
}
// 正常路径：零额外指令（异常表在只读段，不执行）
// 抛异常：栈展开 + 查表 + 析构 → 比正常返回慢 100 倍
```

关键区别：**C 的错误码每次都检查（小开销 × 每次），C++ 的异常正常路径零开销但抛异常极慢（零开销 × 大概率 + 巨大开销 × 小概率）。**

---

## 零开销模型详解

### 正常路径

```
编译器生成的代码（无异常抛出时）：
  call read
  test eax, eax
  jl .throw     ← 如果出错跳到异常代码
  ret           ← 正常返回

异常表（.gcc_except_table 段，只读数据）：
  [PC 范围] → [catch handler 地址] → [需要析构的局部对象]
  ↑ 不执行，只在抛异常时查
```

正常路径只有一条 `jl` 指令（条件跳转），几乎没有开销。

### 抛异常路径

```
throw std::runtime_error("read failed");
  1. 构造异常对象（在栈或堆上）
  2. 查 .gcc_except_table → 找当前 PC 对应的 catch handler
  3. 没找到 → 栈展开：逐帧析构局部对象
  4. 找到 → 跳转到 catch handler
  5. 重复 2-4 直到匹配的 catch

比正常返回慢 2-3 个数量级（可能 1-10μs）
```

### -fno-exceptions

```bash
g++ -O2 -fno-exceptions code.cpp
# 效果：
# - 移除 .gcc_except_table 段（减小二进制）
# - throw/catch 编译错误
# - STL 部分行为变化（如 new 不抛 bad_alloc，返回 nullptr）
# - 确定性更好（无异常的不确定延迟）
```

---

## 常见错误（新手踩坑）

### 错误 1：热路径用异常做控制流

```cpp
void process(int value) {
    if (value < 0) throw std::invalid_argument("negative");
    // 如果 value < 0 是常见情况 → 抛异常极慢
    // 修正：用错误码
}
```

### 错误 2：以为异常完全零开销

```cpp
// 异常正常路径虽几乎零开销，但：
// 1. 二进制体积增大（异常表）
// 2. 可能阻碍某些优化（编译器要生成异常表）
// 3. 抛异常极慢（1-10μs）
```

### 错误 3：-fno-exceptions 后 STL 行为变化

```cpp
// -fno-exceptions 下：
int* p = new int[1000000000];  // 分配失败
// 没有 bad_alloc 异常 → 返回 nullptr（或 std::bad_alloc 行为未定义）
// new(nothrow) 更安全：int* p = new(std::nothrow) int[1000000000];
```

---

## 和 C 的区别

| 特性 | C 错误码 | C++ 异常 |
|------|---------|---------|
| 正常路径开销 | 每次 if 检查 | **几乎零**（一条条件跳转） |
| 错误路径开销 | 和正常路径相同 | **极慢**（100x+） |
| 传播方式 | 手动返回/检查 | 自动栈展开 |
| 资源清理 | 手动 goto cleanup | RAII 自动析构 |
| 可关闭 | N/A | `-fno-exceptions` |
| 二进制 | 小 | 大（异常表） |

---

## HFT 关联

1. **异常零开销模型的真相**：正常路径零开销，但抛异常极慢（1-10μs）——HFT 把异常当"致命错误"用（崩溃重启），不当控制流。
2. **`-fno-exceptions`**：部分 HFT 引擎整体关异常，换二进制体积 + 确定性。但会失去 STL 的异常保证。
3. **热路径用错误码**：可预期的失败（连接超时、参数无效）用错误码——零开销、确定性。异常只用于不可恢复错误。

---

## 代码自测

### Q1: 零开销模型

```cpp
int divide(int a, int b) {
    if (b == 0) throw std::runtime_error("div by zero");
    return a / b;
}
int result = divide(10, 2);
// 正常路径（b != 0）有什么额外开销？
```

<details>
<summary>答案与复习指引</summary>

正常路径几乎零开销——只有一条条件跳转指令（`if (b == 0) goto throw`）。异常表在 .gcc_except_table 只读段，不执行。不抛异常时没有额外指令、没有额外访存。这就是"零开销模型"。

**复习：** → [6.3 异常处理开销](./03-exception-cost.md)
</details>

### Q2: 抛异常代价

```cpp
for (int i = 0; i < 1000; i++) {
    try {
        throw std::runtime_error("test");
    } catch (...) {}
}
// 这个循环比正常返回慢多少？
```

<details>
<summary>答案与复习指引</summary>

慢 100 倍以上。每次抛异常：①构造异常对象 ②查异常表 ③栈展开 ④跳转到 catch。整个过程可能 1-10μs，而正常返回只需几纳秒。**异常绝不能用于热路径控制流。**

**复习：** → [6.3 异常处理开销](./03-exception-cost.md)
</details>

### Q3: 异常 vs 错误码

```cpp
// 方案 A：异常
Result parse(const char* data) {
    if (!data) throw ParseError("null");
    // ...
}

// 方案 B：错误码
int parse(const char* data, Result& out) {
    if (!data) return -1;
    // ...
}
// HFT 热路径选哪个？为什么？
```

<details>
<summary>答案与复习指引</summary>

方案 B（错误码）。理由：①正常路径虽都几乎零开销，但错误码更确定（无异常表查找）；②错误路径错误码远快于异常（几ns vs 几μs）；③错误码不增大二进制。HFT 策略：热路径用错误码，初始化/致命错误用异常。

**复习：** → [6.3 异常处理开销](./03-exception-cost.md)
</details>

### Q4: -fno-exceptions

```bash
# 哪些场景适合 -fno-exceptions？
# A: HFT 交易引擎核心
# B: HFT 守护进程的配置加载
# C: 测试框架
```

<details>
<summary>答案与复习指引</summary>

A 适合（热路径不需要异常，关掉减小二进制 + 确定性）。B 不适合（初始化代码用异常处理配置错误更安全）。C 不适合（测试框架依赖异常验证失败）。策略：核心热路径编译为 `-fno-exceptions`，外围代码保留异常。但混用需小心 ABI 兼容。

**复习：** → [6.3 异常处理开销](./03-exception-cost.md)
</details>

---

## 参考与延伸

- 下一章：[第 7 章 高级对象模型](../ch07-advanced-object-model/README.md)
- 回到：[第 6 章 运行时语义](README.md)
