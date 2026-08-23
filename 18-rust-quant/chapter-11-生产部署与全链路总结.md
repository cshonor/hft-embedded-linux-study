# 第11章 生产部署与全链路总结

> 把 1–10 章收成一张图。然后明确：本模块交付的是地图 + 小 demo，不是可上实盘的系统。

← [第10章](./chapter-10-系统监控与性能调优.md) · 返回 [模块入口](./README.md)

---

## 全链路（和 14 / P10 对齐）

```
Feed ──► 解析/清洗(Ch3) ──► 环(Ch7) ──► Book
                                      ──► Strategy(Ch5)
                                      ──► Risk(Ch9) ──► OMS(Ch8) ──► 交易所
回测(Ch6) 走同一 engine；K 线(Ch4) 只给慢策略
度量(Ch10) 看分位，不看感觉
```

语法、所有权、`Result` 永远回 [17](../17-rust-foundation/)。硬件、bypass、绑核永远回 [14](../14-hft-engineering/)。微观结构（为什么价差存在）回 [19](../19-markets-microstructure/)。

---

## 现在就能跑

WSL（和 P1 / P10 同一习惯）：

```bash
cd /mnt/c/Users/12392/Desktop/hft/18-rust-quant/demo
cargo test
cargo run --release
cargo run --release -- --jump 0
```

| 命令 | 测什么 |
|------|--------|
| `cargo test` | 价格优先、FIFO、STP、IOC/FOK、风控、做市库存、300 tick 回放能跑完 |
| `cargo run --release` | 默认 2 万 tick，带跳价 → 逆向选择 |
| `--jump 0` | 关掉跳价，PnL 通常更好（不是策略变聪明了） |

C++ 全链路对照：

```bash
cd projects/P10-hft-prototype/part-a-demo
make test && ./hft_demo --self-test
./hft_demo
./hft_demo --jump 0
```

两套语义应当能对上：整数 tick、FIFO、本地风控、库存偏斜做市。随机数实现不同，**不要要求 PnL 数字逐 tick 相等**；要比的是方向和规则。

---

## 明确不做

| 不做 | 去哪才做 |
|------|----------|
| DPDK / 共址 / FPGA | 14 Ch5 / Ch13、P7 |
| 真实 ITCH/OUCH | 以后的 P10 终局 |
| 双机热备、监管报送 | 14 Ch11 |
| 无锁 SPSC（`unsafe`） | 14 §7.2；P8 Rust 重写 |
| 从本章学 Rust 语法 | 17 |

上线门禁清单概念上抄 [14 §11.1](../14-hft-engineering/chapter-11-production-deployment-ops/11.1-上线门禁清单.md)：没度量、没拒单计数、没 kill，就当没有实盘。

---

## 建议阅读顺序（新手）

1. 本模块第1章 + [`demo/README`](./demo/README.md)  
2. `cargo test`，对着失败信息回 demo 源码注释  
3. `cargo run --release` 与 `--jump 0`  
4. 第2、5、7、9 章  
5. 对照 P10 `types.hpp` → `orderbook.hpp` → `strategy.hpp` → `engine.hpp`  
6. 再按需读 3、4、6、8、10  

下一站动手：P8 的 Rust 重写可以 **直接抄 demo 的 Book**，再换无锁环。主线若走 Phase1，下一项目是 P2，不是继续扩这个 demo。
