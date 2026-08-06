# demo02 注释嵌套陷阱

`broken.c` 在 `#if 0` 外可编译；若去掉 `#if 0`，会因注释提前结束而报错。

手动复现：新建 `test.c` 仅含：

```c
/* start
   /* inner */
   int x;
*/
```

```bash
gcc -c test.c   # error: expected identifier before '/' token
```

推荐屏蔽代码：`#if 0` … `#endif`。
