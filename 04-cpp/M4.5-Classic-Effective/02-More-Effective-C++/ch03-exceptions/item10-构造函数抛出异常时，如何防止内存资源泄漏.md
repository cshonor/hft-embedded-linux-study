# 条款 10：构造函数抛出异常时，如何防止内存资源泄漏

## 本节讲什么

**Prevent resource leaks in constructors.** 构造函数抛异常有一条特殊规则：
**对象没建成，析构函数不会被调用**——但已经构造完成的**成员**会被析构。
这条规则决定了：构造函数里的裸资源（裸 new 的成员）在异常路径上**必漏**，
而 RAII 成员（unique_ptr 等）天然安全。

← 上一条 [item09 RAII 杜绝泄漏](./item09-利用析构函数杜绝资源泄漏（RAII核心思想）.md)；
下一条 [item11 资源管理函数内的异常](./item11-杜绝资源管理函数内部发生异常造成泄漏.md)。

---

## 1. 规则：构造中断时，只有"已建成员"被清理

```cpp
class Session {
    Socket* sock_;       // 裸指针成员
    Buffer* buf_;
public:
    Session() {
        sock_ = new Socket;      // ① 成功
        buf_ = new Buffer;       // ② 抛异常！
        // Session 构造中断——~Session() 不会被调用
        // sock_ 指向的 Socket **泄漏**（它是裸指针，"成员"只是指针值，不是对象）
    }
};
```

**规则的精确语义**：构造函数抛异常 = 对象从未存在 → 不调析构。
但**已完成构造的成员对象**（成员是类类型）会被逆序析构。
裸指针 `sock_` 只是"指针值"——它指向的堆对象**不是成员对象**，没人替它析构。

对照安全版：

```cpp
class Session {
    std::unique_ptr<Socket> sock_;   // 成员**对象**（unique_ptr 是类类型）
    std::unique_ptr<Buffer> buf_;
public:
    Session() : sock_(std::make_unique<Socket>()),
                buf_(std::make_unique<Buffer>()) {}   // buf_ 抛异常时，
};                                                    // sock_（已建成员）被析构 → 不泄漏
```

**推论**：构造函数可能抛异常的类，成员必须 RAII 化——
"成员的析构替我善后"是构造异常路径上**唯一**的清理机制。

## 2. 初始化列表里的异常（函数 try 块的位置）

成员初始化列表抛异常，普通函数体 catch 接不住——要用**函数 try 块**（→ 18.1 ⑤）：

```cpp
class Session {
    std::string name_;
    Config cfg_;
public:
    Session(const std::string& n) try
        : name_(n), cfg_(load_config(n)) {   // load_config 抛异常 → 这里接
    } catch (const std::exception& e) {
        log("Session 构造失败: ", e.what());
        // ⚠ 构造函数的 catch 块结束时**自动重抛**——
        // 对象没建成，不能"假装成功"（资源由已建成员 name_ 的析构清理）
    }
};
```

要点：函数 try 块的 catch **无法吞掉异常**（结束自动重抛）——
它的用途是**记录/翻译**异常，不是修复。这正好符合语义：构造失败的对象
本来就不该存在。

## 3. 两阶段构造的批判（历史遗留模式）

老代码常用"构造 + init()" 两阶段（构造不做事，init 可能失败返回错误码）——
理由就是"构造不能抛异常"。现代批判：

- 对象存在"已构造未初始化"的**僵尸状态**——每个方法都要防御性检查 `inited_`
- C++ 的答案是反过来：**构造函数该抛就抛**（对象要么完整要么不存在），
  配合 RAII 成员（本条）和工厂函数（返回 `optional`/`expected` 包裹结果）：

```cpp
static std::optional<Session> Session::create(const std::string& n) {
    try { return Session(n); }          // 构造可能抛——工厂接住转 optional
    catch (...) { return std::nullopt; }
}
// 调用方拿不到"半个对象"——要么完整 Session，要么 nullopt
```

## HFT 关联

- 连接/会话对象的构造纪律：**成员全 RAII + 构造函数只做不抛的组装**，
  可能失败的 IO（connect/handshake）挪到工厂/两阶段——
  "对象存在 = 可用"是不变量，让 `if (connected_)` 这类僵尸检查从代码里消失
- 构造失败的**部分成员清理**是 RAII 的免费收益：
  订单簿初始化到一半失败（档位数组建了、索引表没建）——
  已建的 unique_ptr/vector 成员自动回滚，人肉管理早崩了
- 工厂 + optional 的冷路径模式与热路径错误码（→ 18.1 HFT）互补：
  启动/重连（冷）用工厂抛异常，热路径函数绝不抛

## 代码自测

**题目 1：** 构造函数抛异常时，为什么裸指针成员指向的堆对象必漏，而 unique_ptr 成员不漏？

<details>
<summary>参考答案</summary>

构造中断时：**对象本身**从未存在 → 不调析构；
但**已构造完成的成员对象**（类类型成员）会被逆序析构。
裸指针成员只是一个"指针值"——指针指向的堆对象**不是成员对象**，
它的析构没有任何人触发 → 泄漏。
unique_ptr 成员本身是**成员对象**——它已构造完成，
异常时被正常析构，其析构函数释放持有的堆对象 → 不泄漏。
这就是"构造函数会抛异常的类，成员必须 RAII 化"的完整推理。

</details>

**题目 2：** 函数 try 块的 catch 为什么不能"吞掉"异常让对象继续存在？

<details>
<summary>参考答案</summary>

因为构造函数的 catch 块结束时**自动重抛**（语言强制，无法规避）——
这是语义必然：构造函数没跑完 = 对象没建成，
"没建成的对象"不该有生存权（它的不变量可能根本没建立，
让你拿到它是把僵尸对象合法化）。
函数 try 块的正确用途是**记录/翻译**（写日志、换异常类型），
清理工作交给已建成员的析构（RAII）。
想"构造失败还能用默认值"——那是工厂 + optional 的地盘（两阶段模式的现代版）。

</details>

**题目 3：** 两阶段构造（`new Session()` + `init()` 返回错误码）在现代的三个反对理由？

<details>
<summary>参考答案</summary>

① **僵尸状态**：对象存在"已构造未初始化"的中间态——
每个成员函数都得防 `if (!inited_) return E_NOT_INIT;`，防御代码病毒式扩散；
② **违反"构造即成立"的 C++ 契约**：RAII 容器（vector<Session>）、
异常安全（item29）、`make_unique` 全部假设"对象存在即可用"——
两阶段对象放进这些设施里全是地雷；
③ **现代替代品更好**：构造函数该抛就抛（成员 RAII 化后异常路径自动清理），
调用方要错误码语义就用**工厂函数**（`optional<Session> create()`）——
错误处理集中在工厂一处，对象本身永远"存在即可用"。

</details>
