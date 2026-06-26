package main

// Side 买卖方向
type Side int

const (
	Buy Side = iota
	Sell
)

// OrderType 限价单 / 市价单（Harris Ch 4）
type OrderType int

const (
	Limit OrderType = iota
	Market
)

// Order 一条交易指令
type Order struct {
	ID    uint64
	Side  Side
	Type  OrderType
	Price int64 // 价格（ticks）；市价单可填 0
	Qty   int64
	Time  int64 // 同价位内的先后顺序
}
