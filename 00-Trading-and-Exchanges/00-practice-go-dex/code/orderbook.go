package main

// Harris 29 章 ↔ 代码索引：HARRIS-INDEX.md
// 本文件：Ch 4 · Ch 5 · Ch 6（M1–M2）
//   Ch 4 订单类型 → ../chapter-04-orders-and-order-types/
//   Ch 5 市场结构 → ../chapter-05-market-structures/
//   Ch 6 指令驱动 → ../chapter-06-order-driven-markets/
// 概念笔记 → ../notes/milestone-01-订单类型与LOB/section-1-三层结构体解析.md
// ── 三层结构：Order → Limit → Orderbook ─────────────────────────────
// 只存放「还没成交的限价挂单」。市价单吃完对手盘就走，不进 Bids/Asks。

// Order 最小单元：用户一笔限价挂单（未成交部分）
type Order struct {	Size      float64 // 剩余未成交数量
	Limit     *Limit  // 反向指针：这条单挂在哪个价位档位（撮合时快速找到所属 Limit）
	Timestamp int64   // 下单时间；同价位内先下单先成交（时间优先）
}

// Limit 一个固定价格档位（不是「限价单」本身，而是「这个价位的盒子」）
type Limit struct {
	Price       float64   // 档位统一价格，如 50000
	Orders      []*Order  // 同价位所有挂单的指针队列，先来在前（时间优先）
	TotalVolume float64   // 该档剩余总量缓存，避免每次遍历 Orders 求和
}

// Orderbook 顶层容器：整个交易所的挂单簿
type Orderbook struct {
	// Asks []*Limit：卖盘切片。[] 可动态加档位；*Limit 存价位盒子的地址（指针）
	// 价格从低到高，如 [50001, 50002, 50003…]
	Asks []*Limit

	// Bids []*Limit：买盘切片，价格从高到低，如 [50000, 49999, 49998…]
	Bids []*Limit
}

// 撮合直觉（后续在方法里实现）：
// - 限价单：找到/新建对应 Limit → Order 塞进 Limit.Orders → TotalVolume 累加
// - 市价买单：吃 Asks 最低价档，从前到后成交，吃完换下一档
// - 市价卖单：吃 Bids 最高价档
// 规则：价格优先，同价时间优先（Harris Ch 4–6）
