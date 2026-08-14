# Item 2 · 逻辑脉络

← [Item 2 目录](./README.md)

整条链路：**函数 → 方法 → fn → 闭包 → Trait → 静/动态分发**。  
静/动态分发大白话 → [Item 12 §06](../../Chapter-02-Traits/Item-12-generics-vs-trait-objects/06-dispatch-beginner-guide.md)

---

## 一、整体演进链路

```text
自由函数 → 方法（绑定具体类型）
    → 普通 fn（无捕获环境，纯函数指针）
    → 闭包（可捕获环境，实现 Fn* 系列 trait）
    → Trait（统一抽象，实现多态）
         ├─ 泛型 + Trait Bound → 单态化 / 静态分发
         └─ dyn Trait → 虚表(vtable) / 动态分发
```

| 阶段 | 一句话 |
|------|--------|
| **自由函数** | 无归属、不绑定类型，`fn foo()` 独立调用 |
| **方法** | `impl Type` + `self` 变体 → [09-methods-and-self.md](./09-methods-and-self.md) |
| **普通 `fn`** | 函数指针，**不捕获**外部环境，行为固定 |
| **闭包** | 语法简洁，**可捕获**当前作用域变量；编译器生成唯一匿名 struct + 自动 `impl Fn*` |
| **Trait** | 行为契约；多态分 **静态**（编译定址）与 **动态**（运行查 vtable） |

---

## 二、闭包三 Trait 层级与向下兼容

### 定义区分

| Trait | 捕获方式 | 调用 | 典型场景 |
|-------|----------|------|----------|
| **`FnOnce`** | 可**取得所有权** / 消耗环境 | 通常一次 | `move` 闭包、消耗捕获值 |
| **`FnMut`** | **`&mut`** 借用环境 | 可多次，可改 | 计数器、累加 |
| **`Fn`** | **`&`** 只读借用环境 | 可多次，只读 | 读配置、打印 |

继承关系（标准库）：`Fn: FnMut: FnOnce` —— **能力**上 Fn 最强（只读），FnOnce 最弱（可消耗）。

### 兼容规则

> **参数约束越宽（要求越弱），能传入的闭包越多。**

| 参数要求 | 可传入 |
|----------|--------|
| `F: FnOnce` | `Fn` / `FnMut` / `FnOnce` |
| `F: FnMut` | `FnMut` / `Fn` |
| `F: Fn` | 仅 `Fn` |

API 设计：**能写 `FnOnce` 就别写死 `Fn`**，给调用方最大自由。

### 代码示例

```rust
fn call_once<F: FnOnce()>(f: F) { f(); }
fn call_mut<F: FnMut()>(mut f: F) { f(); }
fn call_norm<F: Fn()>(f: F) { f(); }

fn main() {
    // FnOnce 最宽：三种都能传（各用新闭包，因 call_once 会消耗 f）
    let x = 0;
    call_once(|| println!("{x}"));              // Fn ✅

    let mut x = 0;
    call_once(|| { x += 1; });                 // FnMut ✅

    let s = String::from("hi");
    call_once(move || drop(s));                // FnOnce ✅

    // FnMut：FnMut + Fn
    let x = 0;
    call_mut(|| println!("{x}"));              // Fn ✅
    let mut x = 0;
    call_mut(|| { x += 1; });                  // FnMut ✅
    // call_mut(move || { ... });              // 仅 FnOnce ❌

    // Fn：仅 Fn
    let x = 0;
    call_norm(|| println!("{x}"));             // Fn ✅
    // call_norm(|| { x += 1; });             // FnMut ❌
}
```

### 易错点

| 坑 | 说明 |
|----|------|
| **`call_once(f)` 消耗 `f`** | 传进去后闭包往往不能再用；示例里每种测试用**新闭包** |
| **`move` 闭包** | 常是 `FnOnce`；捕获变量被移入闭包 struct |
| **写死 `Fn` 当回调参数** | 合法但过窄；Item 2 主张优先 `FnOnce` |

→ `FnOnce<()>` 尖括号含义：[06-trait-generic-params.md](./06-trait-generic-params.md)

---

## 三、Trait 两种分发机制

| 机制 | 时机 | 典型写法 | 原理 | 优缺 & 场景 |
|------|------|----------|------|-------------|
| **静态分发** | 编译期 | `fn f<T: Trait>(t: T)`<br>`impl Trait` | **单态化**：每具体类型一份专属代码；指令**硬编码跳转地址** | ✅ 零查表、可内联、HFT 热路径<br>❌ 类型多 → **代码膨胀** |
| **动态分发** | 运行期 | `&dyn Trait`<br>`Box<dyn Trait>` | **类型擦除** + **vtable**；胖指针 = 数据 + 表指针；**运行查表**再跳转 | ✅ 异构集合、二进制省<br>❌ 间接调用开销 |

### 补充要点

**1. `impl Trait`（参数 / 返回值）**

静态分发语法糖，编译期确定具体类型，本质同泛型 + bound。

**2. `&dyn Trait` 胖指针（64 位通常 16 字节）**

| 部分 | 指向 |
|------|------|
| 数据指针 | 实例字段（栈/堆） |
| vtable 指针 | **地址登记表**（编译生成，运行查表） |

→ vtable 只存**地址**，方法**实体代码**在代码段；两路最终跳进同一份代码 → [Item 12 §09](../../Chapter-02-Traits/Item-12-generics-vs-trait-objects/06-dispatch-beginner-guide.md)

**3. 异构集合（动态分发刚需）**

```rust
trait Animal { fn speak(&self); }
struct Cat;
struct Dog;
impl Animal for Cat { fn speak(&self) { println!("喵"); } }
impl Animal for Dog { fn speak(&self) { println!("汪"); } }

fn main() {
    let pets: Vec<Box<dyn Animal>> = vec![Box::new(Cat), Box::new(Dog)];
    for p in pets { p.speak(); }

    // ❌ 静态分发：Vec 元素必须是单一具体类型
    // let pets: Vec<impl Animal> = ...;
}
```

**4. 取舍**

| 场景 | 选型 |
|------|------|
| 高频调用、类型编译期已知 | **静态分发**（泛型 / `impl Trait` / `F: FnOnce`） |
| 容器存多种实现、插件、擦除类型 | **动态分发**（`dyn Trait`） |

**5. 对象安全**

不是每个 trait 都能 `dyn`——泛型方法、返回裸 `Self` 等会导致无法建稳定 vtable → [05-pitfalls.md](./05-pitfalls.md)

---

## 四、整条链路串联

```text
1. 自由函数 / 方法     → 组织行为、绑定类型
2. fn vs 闭包           → 要不要捕获环境；闭包走 Fn* 层级
3. Trait                → 抽象「能做什么」
4. 泛型 + bound         → 静态分发，要性能
5. dyn Trait            → 动态分发，要异构 / 灵活
```

**选型口诀**：要性能 → 静态；要同一容器多种类型 → 动态。  
回调 API → **`F: FnOnce`**，少用裸 `fn`（见 [03-key-takeaways.md](./03-key-takeaways.md)）。

---

## 相关

- 核心定义 → [01-core-concepts.md](./01-core-concepts.md)
- 案例代码 → [04-examples.md](./04-examples.md)
- Item 12 深度 → [Item 12 README](../../Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md)
