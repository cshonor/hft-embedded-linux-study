# 17.1 std::any 基础

> 第 17 章 std::any

## 这节讲什么

`std::any` 是类型擦除的容器——可以存任意类型的值，运行时通过 `any_cast` 取回。和 `variant` 不同，`any` 不在编译期列出所有可能类型。

## 基本用法

```cpp
std::any a;

a = 42;              // 存 int
a = 3.14;            // 切换为 double
a = "hello"s;        // 切换为 string

// 检查是否有值
a.has_value();  // true

// 取回
auto x = std::any_cast<int>(a);  // 类型错误抛异常
auto p = std::any_cast<int>(&a); // 安全：错误返回 nullptr

// 清空
a.reset();
```

## any vs variant

| 特性 | `variant<T1,T2,...>` | `any` |
|------|---------------------|-------|
| 类型列表 | 编译期固定 | 任意类型 |
| 类型安全 | 强（编译期检查） | 弱（运行时检查） |
| 内存 | 栈上（max sizeof） | 可能堆分配 |
| 性能 | 快 | 有类型擦除开销 |
| 访问 | `get<T>` / `visit` | `any_cast<T>` |

## 使用场景

### 1. 属性字典

```cpp
std::unordered_map<std::string, std::any> config;
config["port"] = 8080;
config["host"] = "localhost"s;
config["ratio"] = 0.5;

int port = std::any_cast<int>(config["port"]);
std::string host = std::any_cast<std::string>(config["host"]);
```

### 2. 消息总线（弱类型）

```cpp
// 事件系统：任意类型的事件
void publish(const std::string& topic, std::any event);

publish("tick", Tick{12.5, 200});
publish("config_change", Config{...});
```

## HFT 中的角色

`any` 在 HFT 热路径中**不推荐**使用——类型擦除有运行时开销，`any_cast` 做类型检查。HFT 更推荐 `variant`（编译期分发）。

```cpp
// 不推荐：热路径用 any
std::any msg = receive();
if (auto* order = std::any_cast<OrderMsg>(&msg)) { ... }

// 推荐：热路径用 variant
std::variant<OrderMsg, TradeMsg> msg = receive();
std::visit(handler, msg);
```

## 小结

| 接口 | 说明 |
|------|------|
| `a.has_value()` | 是否有值 |
| `a.type()` | 当前类型（type_info） |
| `any_cast<T>(a)` | 取值（错误抛异常） |
| `any_cast<T>(&a)` | 取指针（错误返回 nullptr） |
| `a.reset()` | 清空 |

---

← [本章导读](./README.md)
