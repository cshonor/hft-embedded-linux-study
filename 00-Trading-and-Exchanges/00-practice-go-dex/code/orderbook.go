package main

// 1. 单条用户委托订单
type Order struct {
	Size      float64
	Limit     *Limit
	Timestamp int64
}

// 2. 同一价格档位的订单集合
type Limit struct {
	Price       float64
	Orders      []*Order
	TotalVolume float64
}

// 3. 完整订单簿（承载买盘、卖盘）
type Orderbook struct {
	Asks []*Limit // 卖盘（挂单卖出）
	Bids []*Limit // 买盘（挂单买入）
}
