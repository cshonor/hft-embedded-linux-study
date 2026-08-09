# 条款 40：谨慎使用多重继承

## 本节讲什么

容易出现歧义、菱形继承、成员名冲突；能单继承 + 组合解决就不用多继承。

## 示例

```cpp
class A {};
class B : public A {};
class C : public A {};
class D : public B, public C {};  // 多重继承需谨慎
```

---

## 代码自测

**题目 1：** 菱形继承中，以下代码有什么问题？
```cpp
class File { public: virtual void open() {} };
class InputFile : public File {};
class OutputFile : public File {};
class IOFile : public InputFile, public OutputFile {};
```

<details>
<summary>参考答案</summary>

`IOFile` 中有两份 `File` 子对象（来自 InputFile 和 OutputFile 各一份），导致 `File::open()` 有歧义——`IOFile` 对象上调用 `open()` 编译器不知道走哪条路径。解决：虚继承：
```cpp
class InputFile : virtual public File {};
class OutputFile : virtual public File {};
class IOFile : public InputFile, public OutputFile {};
```
虚继承保证只有一份 `File` 子对象。但虚继承有额外开销（虚基类指针），能不用就不用。

</details>
