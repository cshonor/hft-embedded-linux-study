package main

import "fmt"

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
	Price int64
	Qty   int64
	Time  int64
}

type priceLevel struct {
	price  int64
	orders []*Order
}

// OrderBook 双边限价订单簿
type OrderBook struct {
	bids map[int64]*priceLevel
	asks map[int64]*priceLevel
}

func NewOrderBook() *OrderBook {
	return &OrderBook{
		bids: make(map[int64]*priceLevel),
		asks: make(map[int64]*priceLevel),
	}
}

func (ob *OrderBook) AddLimit(o *Order) error {
	if o.Type != Limit {
		return fmt.Errorf("orderbook: expected limit order")
	}
	if o.Qty <= 0 {
		return fmt.Errorf("orderbook: quantity must be positive")
	}

	levels := ob.bids
	if o.Side == Sell {
		levels = ob.asks
	}

	level, ok := levels[o.Price]
	if !ok {
		level = &priceLevel{price: o.Price}
		levels[o.Price] = level
	}
	level.orders = append(level.orders, o)
	return nil
}

func (ob *OrderBook) BestBid() (int64, bool) {
	return bestPrice(ob.bids, true)
}

func (ob *OrderBook) BestAsk() (int64, bool) {
	return bestPrice(ob.asks, false)
}

func (ob *OrderBook) TakeMarket(side Side, qty int64) (int64, error) {
	if qty <= 0 {
		return 0, fmt.Errorf("orderbook: quantity must be positive")
	}

	var levels map[int64]*priceLevel
	var highestFirst bool
	switch side {
	case Buy:
		levels = ob.asks
		highestFirst = false
	case Sell:
		levels = ob.bids
		highestFirst = true
	default:
		return 0, fmt.Errorf("orderbook: unknown side")
	}

	filled := int64(0)
	for qty > 0 {
		price, ok := bestPrice(levels, highestFirst)
		if !ok {
			break
		}
		level := levels[price]
		for len(level.orders) > 0 && qty > 0 {
			resting := level.orders[0]
			take := min(qty, resting.Qty)
			resting.Qty -= take
			qty -= take
			filled += take
			if resting.Qty == 0 {
				level.orders = level.orders[1:]
			}
		}
		if len(level.orders) == 0 {
			delete(levels, price)
		}
	}
	return filled, nil
}

func bestPrice(levels map[int64]*priceLevel, highestFirst bool) (int64, bool) {
	if len(levels) == 0 {
		return 0, false
	}
	var best int64
	first := true
	for price := range levels {
		if first ||
			(highestFirst && price > best) ||
			(!highestFirst && price < best) {
			best = price
			first = false
		}
	}
	return best, true
}
