# 条款 13：合理使用 catch(...) 全局捕获，谨慎兜底

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
try {
    risky();
} catch (...) {
    log("unknown error");
    throw;  // 谨慎重新抛出
}
```

---

## 代码自测

**题目 1：** 以下两种 catch 方式有什么区别？
```cpp
// 方式A
catch (std::runtime_error& e) { ... }
// 方式B
catch (...) { ... }
```

<details>
<summary>参考答案</summary>

方式A：只捕获 `runtime_error` 及其派生类型，可以访问 `e.what()` 获取错误信息。方式B：捕获所有异常，但无法访问异常对象——通常用于清理资源后 re-throw。`catch(...)` 应谨慎使用：它会吞掉所有异常（包括 `bad_alloc`），可能掩盖真正的错误。通常在 catch(...) 中清理后 `throw;` 重新抛出。

</details>
