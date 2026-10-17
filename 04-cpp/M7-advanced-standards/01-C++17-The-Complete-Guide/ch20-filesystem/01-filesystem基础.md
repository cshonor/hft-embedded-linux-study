# 20.1 文件系统库基础

> 第 20 章 文件系统库

## 这节讲什么

C++17 终于把文件系统库纳入标准（`<filesystem>`）。之前要用 Boost.Filesystem 或 POSIX API。`std::filesystem` 提供跨平台的文件/目录操作。

## 基本用法

### 路径操作

```cpp
#include <filesystem>
namespace fs = std::filesystem;

// 创建路径
fs::path p = "/data/hft/config.json";
p.parent_path();    // "/data/hft"
p.filename();       // "config.json"
p.stem();           // "config"（不含扩展名）
p.extension();      // ".json"

// 拼接
fs::path dir = "/data/hft";
fs::path file = dir / "config.json";  // 使用 / 运算符拼接
// file = "/data/hft/config.json"

// 检查
fs::exists(p);        // 文件是否存在
fs::is_regular_file(p);  // 是普通文件吗
fs::is_directory(p);     // 是目录吗
```

### 文件操作

```cpp
// 创建目录
fs::create_directory("data");
fs::create_directories("data/hft/logs");  // 递归创建

// 删除
fs::remove("data/temp.txt");
fs::remove_all("data/old");  // 递归删除

// 复制
fs::copy("src.txt", "dst.txt");
fs::copy("src_dir", "dst_dir", fs::copy_options::recursive);

// 移动/重命名
fs::rename("old.txt", "new.txt");

// 文件大小
auto size = fs::file_size("data.bin");
```

### 目录遍历

```cpp
// 非递归遍历
for (const auto& entry : fs::directory_iterator("data")) {
    std::cout << entry.path() << "\n";
}

// 递归遍历
for (const auto& entry : fs::recursive_directory_iterator("data")) {
    if (entry.is_regular_file()) {
        std::cout << entry.path() << " (" << entry.file_size() << " bytes)\n";
    }
}
```

### 文件属性

```cpp
auto perms = fs::status("data.bin").permissions();

// 修改权限
fs::permissions("data.bin", fs::perms::owner_read | fs::perms::owner_write);

// 最后修改时间
auto ftime = fs::last_write_time("data.bin");
```

## HFT 关联

```cpp
// 加载配置文件
std::optional<Config> load_config(const fs::path& path) {
    if (!fs::exists(path)) return std::nullopt;
    return parse(read_file(path));
}

// 日志轮转：按日期组织日志
fs::path log_dir = "logs/" + today_date();
fs::create_directories(log_dir);
auto log_file = log_dir / ("hft_" + session_id() + ".log");

// 遍历回测数据文件
std::vector<fs::path> find_data_files(const fs::path& dir) {
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".csv") {
            files.push_back(entry.path());
        }
    }
    return files;
}
```

## 小结

| 操作 | 接口 |
|------|------|
| 路径拼接 | `dir / "file.txt"` |
| 检查存在 | `fs::exists(p)` |
| 创建目录 | `fs::create_directories(p)` |
| 删除 | `fs::remove(p)` / `fs::remove_all(p)` |
| 遍历 | `fs::directory_iterator(p)` |
| 递归遍历 | `fs::recursive_directory_iterator(p)` |

---

← [本章导读](./README.md)
