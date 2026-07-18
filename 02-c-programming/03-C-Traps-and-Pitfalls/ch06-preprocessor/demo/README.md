# ch06 Demo

```bash
make all
./demo01_parens/main
./demo02_side_effect/main
./demo03_dowhile/main
make -C demo03_dowhile bad          # BAD_STEP breaks if/else — compile error
./demo04_stringify/main
./demo05_token_paste/main
./demo06_cond_compile/main
make -C demo06_cond_compile debug=1 clean all
./demo07_max/main
```
