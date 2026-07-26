# 9.6 总结

> 章级精读：[../study.md#ch09-exam](../study.md#ch09-exam)

## 本节核心目标

对比广播 vs 组播 vs IGMP/MLD 角色。

---

## 对照

| | 广播 | 组播 |
|--|------|------|
| CPU 代价 | 子网内**全员**处理 | 仅订阅者（+ 映射误收需过滤） |
| 控制协议 | 无 | **IGMP / MLD** |
| 交换机 | 泛洪 | **Snooping** 精准转发 |

---

## 下一章

- [ch10 UDP 与 IP 分片](../../chapter10-udp-ip-fragment/study.md)
