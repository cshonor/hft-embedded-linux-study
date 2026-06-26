package order

// Side is the buy or sell direction of an order.
type Side int

const (
	Buy Side = iota
	Sell
)

// Type distinguishes limit and market orders (Harris Ch 4).
type Type int

const (
	Limit Type = iota
	Market
)

// Order is a single instruction with price–time priority fields for LOB placement.
type Order struct {
	ID    uint64
	Side  Side
	Type  Type
	Price int64 // ticks; ignored for market orders
	Qty   int64
	Time  int64 // sequence timestamp for time priority at the same price
}
