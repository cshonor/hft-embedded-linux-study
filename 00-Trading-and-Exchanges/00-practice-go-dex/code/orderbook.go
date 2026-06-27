package main

// Harris 29 章 ↔ 代码索引：HARRIS-INDEX.md
// 本文件：Ch 4 · Ch 5 · Ch 6（M1–M2）
//   Ch 4 订单类型 → ../chapter-04-orders-and-order-types/
//   Ch 5 市场结构 → ../chapter-05-market-structures/
//   Ch 6 指令驱动 → ../chapter-06-order-driven-markets/
// 概念笔记 → ../notes/milestone-01-订单类型与LOB/section-1-三层结构体解析.md
// ── 三层结构：Order → Limit → Orderbook ─────────────────────────────
// 交易所「大卖场」里的基础收银台：现货 LOB 撮合（Ch 1 §3）。
// M1–M2 = 散户级单层公开簿（市价/限价）；多层 MM/ECN 路由 (SOR) → M5+（Ch 2 §1 Level II）。
// 只存放「还没成交的限价挂单」。市价单吃完对手盘就走，不进 Bids/Asks。
// 衍生品区（保证金、行权、爆仓）→ 叠在本引擎外层，不混进 Match() 核心。

// OrderType 散户最常用的两种单 — Match() 入口先分支（Ch 2 §1 · Ch 4）
type OrderType int

const (
	OrderLimit OrderType = iota // 限价：进簿排队，价格–时间优先
	OrderMarket                 // 市价：立刻 walk-the-book，不进 Bids/Asks
)

// Side 买 / 卖
type Side int

const (
	Buy Side = iota
	Sell
)

// Order 最小单元：用户一笔订单（限价未成交部分挂在 Limit 上）
type Order struct {
	OrderType OrderType
	Side      Side
	Price     float64 // 限价；市价单在撮合时用对手档价格
	Size      float64 // 剩余未成交数量
	Limit     *Limit  // 反向指针：限价单挂在哪个价位档（市价单通常为 nil）
	Timestamp int64   // 下单时间；同价位内先下单先成交（时间优先）
	Owner     string  // "retail" / "mm" — 做市商模块用来撤换自己的单
}

// Limit 一个固定价格档位（不是「限价单」本身，而是「这个价位的盒子」）
type Limit struct {
	Price       float64
	Orders      []*Order
	TotalVolume float64
}

// Orderbook 顶层容器：整个交易所的挂单簿
type Orderbook struct {
	Asks []*Limit // 卖盘，价低在前
	Bids []*Limit // 买盘，价高在前
}

// Trade 一笔成交
type Trade struct {
	Price     float64
	Size      float64
	Taker     *Order
	Maker     *Order
	Timestamp int64
}

// BestBid 全簿最高买价（Level 1 bid）
func (ob *Orderbook) BestBid() (price, size float64, ok bool) {
	if len(ob.Bids) == 0 {
		return 0, 0, false
	}
	lim := ob.Bids[0]
	return lim.Price, lim.TotalVolume, true
}

// BestAsk 全簿最低卖价（Level 1 ask）
func (ob *Orderbook) BestAsk() (price, size float64, ok bool) {
	if len(ob.Asks) == 0 {
		return 0, 0, false
	}
	lim := ob.Asks[0]
	return lim.Price, lim.TotalVolume, true
}

// Spread Level 1 买卖价差；簿不全时 ok=false
func (ob *Orderbook) Spread() (spread float64, ok bool) {
	bid, _, bidOK := ob.BestBid()
	ask, _, askOK := ob.BestAsk()
	if !bidOK || !askOK {
		return 0, false
	}
	return ask - bid, true
}

// AddOrder 入口：先 OrderType 分支，再价格–时间优先撮合；可成交部分立刻成交，限价剩余进簿
func (ob *Orderbook) AddOrder(o *Order) []Trade {
	if o.OrderType == OrderMarket {
		return ob.matchMarket(o)
	}
	return ob.matchLimit(o)
}

func (ob *Orderbook) matchMarket(taker *Order) []Trade {
	var trades []Trade
	for taker.Size > 0 {
		var lim *Limit
		if taker.Side == Buy {
			if len(ob.Asks) == 0 {
				break
			}
			lim = ob.Asks[0]
		} else {
			if len(ob.Bids) == 0 {
				break
			}
			lim = ob.Bids[0]
		}
		tr := ob.fillAtLimit(taker, lim)
		if tr == nil {
			break
		}
		trades = append(trades, *tr)
	}
	return trades
}

func (ob *Orderbook) matchLimit(o *Order) []Trade {
	var trades []Trade
	for o.Size > 0 && ob.canCross(o) {
		var lim *Limit
		if o.Side == Buy {
			lim = ob.Asks[0]
		} else {
			lim = ob.Bids[0]
		}
		tr := ob.fillAtLimit(o, lim)
		if tr == nil {
			break
		}
		trades = append(trades, *tr)
	}
	if o.Size > 0 {
		ob.restLimit(o)
	}
	return trades
}

func (ob *Orderbook) canCross(o *Order) bool {
	if o.Side == Buy {
		if len(ob.Asks) == 0 {
			return false
		}
		return o.Price >= ob.Asks[0].Price
	}
	if len(ob.Bids) == 0 {
		return false
	}
	return o.Price <= ob.Bids[0].Price
}

func (ob *Orderbook) fillAtLimit(taker *Order, lim *Limit) *Trade {
	if len(lim.Orders) == 0 {
		ob.removeEmptyLimit(taker.Side.opposite(), lim)
		return nil
	}
	maker := lim.Orders[0]
	size := taker.Size
	if maker.Size < size {
		size = maker.Size
	}
	taker.Size -= size
	maker.Size -= size
	lim.TotalVolume -= size

	tr := &Trade{
		Price:     lim.Price,
		Size:      size,
		Taker:     taker,
		Maker:     maker,
		Timestamp: taker.Timestamp,
	}

	if maker.Size == 0 {
		lim.Orders = lim.Orders[1:]
	}
	if lim.TotalVolume == 0 {
		ob.removeEmptyLimit(taker.Side.opposite(), lim)
	}
	return tr
}

func (ob *Orderbook) restLimit(o *Order) {
	limits := ob.sideLimits(o.Side)
	idx := findLimitIndex(limits, o.Price, o.Side)
	if idx < len(limits) && limits[idx].Price == o.Price {
		lim := limits[idx]
		lim.Orders = append(lim.Orders, o)
		lim.TotalVolume += o.Size
		o.Limit = lim
		return
	}
	lim := &Limit{Price: o.Price, Orders: []*Order{o}, TotalVolume: o.Size}
	o.Limit = lim
	if o.Side == Buy {
		ob.Bids = insertLimit(ob.Bids, lim, idx, Buy)
	} else {
		ob.Asks = insertLimit(ob.Asks, lim, idx, Sell)
	}
}

func (ob *Orderbook) sideLimits(side Side) []*Limit {
	if side == Buy {
		return ob.Bids
	}
	return ob.Asks
}

func (ob *Orderbook) removeEmptyLimit(side Side, lim *Limit) {
	limits := ob.sideLimits(side)
	for i, l := range limits {
		if l == lim {
			if side == Buy {
				ob.Bids = append(limits[:i], limits[i+1:]...)
			} else {
				ob.Asks = append(limits[:i], limits[i+1:]...)
			}
			return
		}
	}
}

func (ob *Orderbook) CancelByOwner(owner string) {
	ob.Bids = cancelOwnerOnSide(ob.Bids, owner)
	ob.Asks = cancelOwnerOnSide(ob.Asks, owner)
}

func cancelOwnerOnSide(limits []*Limit, owner string) []*Limit {
	out := limits[:0]
	for _, lim := range limits {
		kept := lim.Orders[:0]
		for _, o := range lim.Orders {
			if o.Owner != owner {
				kept = append(kept, o)
			}
		}
		if len(kept) == 0 {
			continue
		}
		lim.Orders = kept
		lim.TotalVolume = 0
		for _, o := range kept {
			lim.TotalVolume += o.Size
		}
		out = append(out, lim)
	}
	return out
}

func (s Side) opposite() Side {
	if s == Buy {
		return Sell
	}
	return Buy
}

func findLimitIndex(limits []*Limit, price float64, side Side) int {
	for i, lim := range limits {
		if side == Buy {
			if price > lim.Price {
				return i
			}
		} else {
			if price < lim.Price {
				return i
			}
		}
	}
	return len(limits)
}

func insertLimit(limits []*Limit, lim *Limit, idx int, side Side) []*Limit {
	limits = append(limits, nil)
	copy(limits[idx+1:], limits[idx:])
	limits[idx] = lim
	return limits
}
