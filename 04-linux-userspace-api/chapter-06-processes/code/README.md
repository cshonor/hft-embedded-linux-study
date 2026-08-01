# Ch6 demos

```bash
cc -Wall -Wextra -o t_getenv t_getenv.c && ./t_getenv | head
cc -Wall -Wextra -O2 -o setjmp_vars setjmp_vars.c && ./setjmp_vars
cc -Wall -Wextra -o mem_segments mem_segments.c && ./mem_segments
```

| 文件 | 对应 |
|------|------|
| `t_getenv.c` | Listing 6-1 · 遍历 `environ` |
| `setjmp_vars.c` | Listing 6-2 · `volatile` 与 `longjmp` |
| `mem_segments.c` | Listing 6-3 · 地址空间各段地址 |
