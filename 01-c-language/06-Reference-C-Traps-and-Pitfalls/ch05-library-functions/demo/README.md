# ch05 Demo

```bash
make all
./demo01_printf/main
./demo02_strncpy/main
./demo03_strcpy/main          # guard 被覆盖
./demo04_malloc/main
make -C demo04_malloc double_free   # 双重 free（可能 abort）
./demo05_ctype/main
./demo06_fgets/main
./demo07_math/main
```
