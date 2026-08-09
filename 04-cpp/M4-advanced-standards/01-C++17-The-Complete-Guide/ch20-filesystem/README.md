# 第 20 章 文件系统库

**The Filesystem Library**

## 本章讲什么

C++17 终于把 `<filesystem>` 纳入标准库（源自 `boost::filesystem`），提供跨平台的路径操作、目录遍历、文件属性查询。替代了手写 `opendir`/`stat`/`mkdir` 的平台依赖代码。

## 要点

### 核心类型

```cpp
#include <filesystem>
namespace fs = std::filesystem;

fs::path p = "/data/quotes/2026-01-01.csv";
p / "subdir";            // / 运算符拼接路径
p.filename();            // "2026-01-01.csv"
p.stem();                // "2026-01-01"
p.extension();           // ".csv"
p.parent_path();         // "/data/quotes"
p.replace_extension(".bin");
```

### 路径操作

```cpp
fs::path p = "dir/file.txt";
fs::exists(p);              // 是否存在
fs::is_regular_file(p);     // 是普通文件
fs::is_directory(p);        // 是目录
fs::file_size(p);           // 大小（字节）
fs::current_path();         // 当前工作目录
fs::absolute(p);            // 绝对路径
fs::canonical(p);           // 规范化（解析符号链接）
```

### 目录遍历

```cpp
// 1. 非递归
for (const auto& entry : fs::directory_iterator("/data/quotes")) {
    std::cout << entry.path() << '\n';
}

// 2. 递归
for (const auto& entry : fs::recursive_directory_iterator("/data")) {
    if (entry.is_regular_file() && entry.path().extension() == ".csv") {
        process(entry.path());
    }
}

// 3. 错误处理（不抛异常版）
std::error_code ec;
for (const auto& entry : fs::directory_iterator("/data", ec)) { ... }
```

### 文件操作

```cpp
fs::create_directory("newdir");
fs::create_directories("a/b/c");     // 递归创建
fs::remove("file.txt");              // 删除文件
fs::remove_all("dir");               // 递归删除
fs::rename("old", "new");
fs::copy("src", "dst", fs::copy_options::recursive);
```

### 错误处理两种方式

```cpp
// 1. 抛异常
try { fs::file_size(p); }
catch (const fs::filesystem_error& e) { ... }

// 2. error_code（不抛）
std::error_code ec;
auto size = fs::file_size(p, ec);
if (ec) { /* 处理错误 */ }
```

热路径或不能抛异常的场景用 `error_code` 版本。

## HFT 关联

- **行情数据加载**：盘前批量加载历史行情用 `recursive_directory_iterator` 遍历 `/data/quotes/YYYY-MM-DD/`，按日期/合约筛选文件。
- **配置文件读取**：策略配置用 `fs::path` 构造路径，`is_regular_file` 检查存在性，跨平台无需 `#ifdef`。
- **日志文件管理**：日志轮转检查 `file_size` + `rename`，用 `error_code` 版避免异常。
- **热路径不用 filesystem**：`<filesystem>` 有内部锁和系统调用，热路径不用——盘前/盘后加载用。
- **符号链接处理**：`canonical` 解析符号链接，避免 `/data` 是软链到 `/ssd/data` 时路径混乱。
- **跨平台**：Windows 路径分隔符是 `\`，`fs::path` 自动处理，代码无需 `#ifdef _WIN32`。

## 自测题

1. `fs::path` 的 `/` 运算符做什么？`stem()` 和 `extension()` 的区别？
2. `directory_iterator` 和 `recursive_directory_iterator` 的区别？
3. filesystem 操作的两种错误处理方式是什么？热路径用哪种？
4. 为什么 HFT 热路径不用 `<filesystem>`？它有什么开销？
5. HFT 盘前加载历史行情如何用 `recursive_directory_iterator` 按日期筛选？

## 代码自测

### Q1: filesystem 基本操作
```cpp
namespace fs = std::filesystem;

fs::path p = "/data/orders/2024";
fs::create_directories(p);  // 递归创建

for (const auto& entry : fs::directory_iterator(p)) {
    if (entry.path().extension() == ".csv") {
        std::cout << entry.path().filename() << '\n';
    }
}

auto size = fs::file_size(p / "orders.csv");
bool exists = fs::exists(p);
```
> std::filesystem 相比 POSIX API（mkdir/opendir/stat）有什么优势？

<details>
<summary>答案与复习指引</summary>

**优势**：
1. **跨平台**：Windows/POSIX 统一接口
2. **类型安全**：`fs::path` 封装路径，自动处理分隔符（`/` vs `\`）
3. **异常安全**：支持 error_code 重载（不抛异常）
4. **高级操作**：`create_directories`（递归）、`directory_iterator`（遍历）、`file_size`/`last_write_time`

**HFT 注意**：filesystem 操作有系统调用开销，不在热路径使用。启动时加载配置/数据文件可用。`directory_iterator` 可能有缓存问题（NFS 等网络文件系统延迟）。

**复习：** → [filesystem](./README.md)
</details>
