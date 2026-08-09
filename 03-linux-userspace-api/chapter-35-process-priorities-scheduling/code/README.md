# Ch35 demos — Priorities & scheduling

```bash
cc -Wall -Wextra -o t_nice t_nice.c
./t_nice

cc -Wall -Wextra -o sched_view sched_view.c
./sched_view
# optional (needs CAP_SYS_NICE): ./sched_view fifo 10

cc -Wall -Wextra -D_GNU_SOURCE -o affinity_demo affinity_demo.c
./affinity_demo
```

| 文件 | 说明 |
|------|------|
| `t_nice.c` | get/setpriority |
| `sched_view.c` | 查看策略；可选设 SCHED_FIFO |
| `affinity_demo.c` | CPU 亲和读写 |
