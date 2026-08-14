# 4. Conditional Compilation（条件编译）

> 所属：**Conditional Compilation** · [← 章索引](./README.md)

← [05 构建配置](./05-build-configuration.md) · 下一节 [07 MSRV](./07-msrv.md)

前置 → [01 Feature](./01-defining-including-features.md) · [02 crate 内使用 Feature](./02-using-features-in-crate.md)

Book → [11.1.1 cfg test](../../00-Book/11-testing/11.1.1-cfg-test与模拟输入.md)

---

## 一、核心概念

**`#[cfg(predicate)]`** — 条件不成立时，整条代码项**从源码移除**，不参与编译、不进二进制。

| | `#[cfg]` | 运行时 `if` |
|---|----------|-------------|
| 编译 | 不满足 → **删除** | 两分支都编译 |
| 运行时 | 无开销 | 分支判断 |

### 配套工具

| 工具 | 作用 |
|------|------|
| **`#[cfg_attr(cond, attr)]`** | 条件附加属性（如 feature 开启才 derive Serialize） |
| **`cfg!(cond)`** | 编译期 bool — **代码保留**，分支被优化消除 |
| **`[target.'cfg(...)'.dependencies]`** | Cargo 平台 / 条件专属依赖 |

---

## 二、四大类常用 Predicate

### 1. 平台 / 目标硬件（跨平台核心）

| 语法 | 含义 |
|------|------|
| `#[cfg(windows)]` | Windows |
| `#[cfg(unix)]` | Linux / macOS / BSD 等 |
| `#[cfg(target_os = "linux")]` | Linux |
| `#[cfg(target_os = "macos")]` | macOS |
| `#[cfg(target_arch = "x86_64")]` | x64 |
| `#[cfg(target_arch = "aarch64")]` | ARM64 |
| `#[cfg(target_pointer_width = "64")]` | 64 位 |
| `#[cfg(target_env = "msvc")]` | Windows MSVC |

```rust
#[cfg(target_os = "windows")]
const SEP: &str = r"\";
#[cfg(unix)]
const SEP: &str = "/";
```

### 2. 测试专用 — `#[cfg(test)]`

- 仅 **`cargo test`** 时启用  
- 单元测试 mod、测试辅助、测试依赖  
- **`cargo build`** 会删除  

```rust
#[cfg(test)]
mod tests {
    #[test]
    fn demo() {}
}
```

### 3. Cargo Feature — `#[cfg(feature = "serde")]`

配合 [01 / 02 Feature](./01-defining-including-features.md)：

```rust
#[derive(Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Config;

#[cfg(feature = "serde")]
pub fn to_json(&self) -> String { /* … */ }
```

### 4. 编译环境内置标记

| 条件 | 说明 |
|------|------|
| **`debug_assertions`** | Debug 模式 — `debug_assert!` 等 |
| **`not(test)`** | 排除测试环境 — 仅生产逻辑 |
| **`nightly`**（自定义） | 需 `RUSTFLAGS="--cfg nightly"` 注入 |

---

## 三、逻辑组合：`all` / `any` / `not`

```rust
#[cfg(all(unix, target_pointer_width = "64"))]
fn unix64() {}

#[cfg(any(windows, target_os = "macos"))]
fn desktop() {}

#[cfg(not(test))]
fn prod_logic() {}

#[cfg(all(target_os = "linux", feature = "serde"))]
fn linux_serde() {}
```

---

## 四、Cargo.toml 条件依赖

```toml
[target.'cfg(windows)'.dependencies]
winapi = "0.3"

[target.'cfg(unix)'.dependencies]
libc = "0.2"
nix = "0.26"

[target.'cfg(target_pointer_width = "64")'.dependencies]
simd = "0.2"

[target.'cfg(all(unix, target_arch = "aarch64"))'.dependencies]
arm_lib = "1.0"
```

> 平台依赖写在 **`[target.'cfg(...)'.dependencies]`** — 不要全局引入再用 `cfg` 屏蔽。

---

## 五、`cfg` / `cfg_attr` / `cfg!` 三者区分

| | 行为 |
|---|------|
| **`#[cfg(cond)]`** | 不满足 → **删除整条 item**（fn / struct / mod） |
| **`#[cfg_attr(cond, Derive)]`** | 结构体保留；条件成立才附加属性 |
| **`cfg!(cond)`** | 编译期 bool；**整段代码保留** |

```rust
fn print_os() {
    if cfg!(windows) {
        println!("win");
    } else {
        println!("unix");
    }
}
```

---

## 六、工程规范与避坑

| # | 规范 |
|:-:|------|
| 1 | 跨平台实现**各自** `#[cfg]` — 避免无匹配项编译失败 |
| 2 | 可选 feature 收拢到 **`#[cfg(feature)] mod`** |
| 3 | CI：`--no-default-features` + `--all-features` → [02](./02-using-features-in-crate.md) |
| 4 | 平台依赖用 **`target.'cfg()'.dependencies`** |
| 5 | **`cfg(test)`** 辅助勿混入生产二进制 |
| 6 | 自定义 `--cfg xxx` 经 **`RUSTFLAGS` / `build.rs`** — **不能**用 Cargo feature 生成 |

### 与 C `#ifdef` 区别

Rust `cfg` 是**编译器原生语义**（非文本预处理器）；未选中分支仍受**语法 / 类型检查**（在 cfg 为 false 时该 item 不参与检查）。

---

## 七、核心速记

1. **`#[cfg]`** 不满足 → 删除代码，零运行开销；**`cfg!`** 代码保留。  
2. 三大条件：**平台** · **test** · **feature**；`all/any/not` 嵌套。  
3. **`#[cfg_attr]`** — 条件 derive 的标准写法。  
4. **`[target.'cfg(...)'.dependencies]`** — 平台专属依赖。  
5. 非 C 预处理器 — 编译器级裁剪。

---

## 速记

## cfg vs if

`#[cfg]` 不满足 → **删代码** · `if` 两分支都编译

## 三大 Predicate

平台 `target_*` · `test` · `feature = "x"`

## 组合

`all()` · `any()` · `not()`

## 三工具

| | |
|---|---|
| `#[cfg]` | 删 item |
| `#[cfg_attr]` | 条件属性 |
| `cfg!()` | 保留代码，编译期 bool |

## Cargo 侧

```toml
[target.'cfg(unix)'.dependencies]
libc = "0.2"
```

## 避坑

feature 收 mod · CI 双构建 · 平台 dep 别全局引 · `--cfg` 非 feature

## 自测

- [ ] `cfg!(windows)` 和 `#[cfg(windows)] fn f()` 差在哪？  
- [ ] 为何 `#[cfg(test)] mod tests` 不进 release 二进制？  
- [ ] 自定义 `nightly` cfg 怎么注入？

