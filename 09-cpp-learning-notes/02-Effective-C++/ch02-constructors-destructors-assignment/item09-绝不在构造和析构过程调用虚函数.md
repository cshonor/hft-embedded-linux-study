# 条款 9：绝不在构造和析构过程调用虚函数

## 本节讲什么

**Never call virtual functions during construction or destruction.** 构造/析构期间调用 virtual，**绝不会**动态绑定到派生类版本，只会调用**当前正在执行的那个 ctor/dtor 所属类**的版本。此时派生部分未就绪或已销毁，若「假多态」向下调用 → 未初始化成员、逻辑错乱、纯虚调用 abort。

← [条款 8：析构与异常](./item08-别让异常逃离析构函数.md) · [条款 7：virtual 析构](./item07-多态基类声明virtual析构函数.md)

---

## 1. 为什么构造/析构期间不能「真多态」？

### 构造顺序

派生类对象创建：**基类子对象先构造 → 再构造派生部分**。

在 **基类构造函数** 运行期间：

- 派生类数据成员**尚未初始化**；
- 若 virtual 调用能落到派生类 override → 可能读写**未初始化**成员 → **UB**；
- 因此 C++ **禁止**这种动态绑定。

此时对象在语言眼里就是 **基类类型**：

| 机制 | 基类 ctor 期间的行为 |
|------|---------------------|
| **virtual 调用** | 解析为 **基类** 版本 |
| **`dynamic_cast`** | 视为基类子对象 |
| **`typeid`** | 报告基类类型 |

### 析构顺序

**派生析构先执行 → 基类析构后执行**。进入 **基类析构** 时，派生成员已处于**未定义状态**，对象再次被视为**基类** → virtual 仍只走基类版。

```cpp
class Base {
public:
    Base() { log(); }           // ❌ 调 virtual
    virtual ~Base() { log(); }  // ❌ 析构里同样只调 Base::log
    virtual void log() const { /* Base */ }
};

class Derived : public Base {
public:
    virtual void log() const override { /* 想用 Derived 版 —  ctor/dtor 里永远到不了 */ }
};
```

**记住：构造/析构里执行的 virtual，绝对不是派生类 override。**

---

## 2. 隐蔽陷阱：`init()` 里的间接 virtual 调用

直接在 ctor 里写 `virtual` 调用，好编译器可能警告； refactor 成 **`init()`** 再在 `init()` 里调 virtual → **往往无警告**，更阴险：

```cpp
class Base {
public:
    Base() { init(); }            // 看起来干净
private:
    void init() { log(); }        // 间接 virtual — 仍是 Base::log
    virtual void log() = 0;       // 纯虚 → 可能直接 abort
};
```

| 被调用的 virtual | 运行时常见结果 |
|------------------|----------------|
| **纯 virtual** | 程序异常终止 |
| **有默认实现的 virtual** | 静默走 **基类版** → 逻辑与预期不符 |

**规范：init 里也不要调 virtual；更不要在 init 里做依赖派生类状态的任何事。**

---

## 3. 替代方案：派生类向基类「向上传参」

不能在基类构造里用 virtual **向下拉**派生类信息 → 改为派生 ctor **把信息传给基类 ctor**。

### 用 `private static` 辅助函数生成基类参数

`static` 不访问**尚未存在的**派生类实例成员，只根据编译期/入参生成字符串等：

```cpp
class Transaction {
public:
    explicit Transaction(const std::string& logInfo);
    void logTransaction() const;   // 普通成员：对象完全构造后再调 virtual
    virtual void log() const = 0;
};

class BuyTransaction : public Transaction {
public:
    BuyTransaction(/* ... */)
        : Transaction(createLogString(/* 来自 ctor 参数，非成员 */))
    {}
private:
    static std::string createLogString(/* ... */) {
        // static：不碰 BuyTransaction 未初始化的成员
        return /* 拼 log 字符串 */;
    }
};
```

| 阶段 | 做什么 |
|------|--------|
| **构造** | 派生 static 辅助 → 基类 ctor 参数；**不调** virtual |
| **构造完成后** | 客户调 `logTransaction()` → 此时才 **多态** 调 `BuyTransaction::log` |

把原来的 virtual **改成非 virtual 的普通函数**（如 `logTransaction()`），在**对象完全建成之后**再调用。

---

## 与条款 7 的对比

| 条款 | 场景 |
|------|------|
| **7** | 通过**基类指针 delete** → 需要 **virtual 析构** |
| **9** | **正在执行** 某层 ctor/dtor 时 → **不要指望 virtual 多态** |

多态在**对象生命周期中间**才可靠；**诞生与消亡**的两端没有「完整派生对象」。

---

## HFT 视角

- **不要在基类 ctor 里 `virtual initConnection()`** 指望网关派生类连不同交易所 —— 只会连到基类默认逻辑。
- **策略基类**：构造只收 **config 字符串 / enum**（由派生 static 工厂拼好）；**注册回调、订阅行情**放到 `start()`（非 virtual 或完全构造后调 virtual）。
- 析构里同样：**不要 virtual cleanup()**；用 [条款 8](./item08-别让异常逃离析构函数.md) 的 `close()` + 非抛析构后备。

---

## 易错点速查

| 坑 | 做法 |
|----|------|
| ctor 末尾调 `virtual init()` | 改为基类 ctor 参数 + 构造后 `start()` |
| 以为析构会调派生 `~X` 里的 virtual | 基类 dtor 阶段只有基类 dynamic 类型 |
| static 辅助却访问派生成员 | static 只用 **参数**，不用 `this` 成员 |
| 纯虚在 ctor 链中被间接调用 | 直接 terminate — 查 init 是否调 virtual |

---

## 核心总结

1. 构造/析构期间 **virtual → 当前类的版本**， never 派生 override。
2. 此时 **typeid / dynamic_cast** 也当基类看。
3. **`init()` 间接调用** 同样危险，编译器常不警告。
4. **替代**：派生 **static 辅助** + **基类 ctor 参数**；多态放到 **对象建完以后** 的普通/虚函数里。
5. 需要定制 → **向上传参**，不要向下 virtual。

← [条款 8：析构与异常](./item08-别让异常逃离析构函数.md) | [条款 10：operator= 返回 *this →](./item10-令operator=返回this引用.md)
