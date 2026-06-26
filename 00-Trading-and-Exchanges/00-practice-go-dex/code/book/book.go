package book

import (
	"fmt"

	"github.com/cshonor/practice-go-dex/order"
)

// priceLevel holds resting limit orders at one price (FIFO = time priority).
type priceLevel struct {
	price  int64
	orders []*order.Order
}

// OrderBook is an in-memory bilateral limit order book (M1: single-threaded).
type OrderBook struct {
	bids map[int64]*priceLevel // highest price first when queried
	asks map[int64]*priceLevel // lowest price first when queried
}

// New returns an empty order book.
func New() *OrderBook {
	return &OrderBook{
		bids: make(map[int64]*priceLevel),
		asks: make(map[int64]*priceLevel),
	}
}

// AddLimit inserts a resting limit order into the book.
func (ob *OrderBook) AddLimit(o *order.Order) error {
	if o.Type != order.Limit {
		return fmt.Errorf("book: expected limit order, got %v", o.Type)
	}
	if o.Qty <= 0 {
		return fmt.Errorf("book: quantity must be positive")
	}

	levels := ob.bids
	if o.Side == order.Sell {
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

// BestBid returns the highest bid price, if any.
func (ob *OrderBook) BestBid() (int64, bool) {
	return bestPrice(ob.bids, true)
}

// BestAsk returns the lowest ask price, if any.
func (ob *OrderBook) BestAsk() (int64, bool) {
	return bestPrice(ob.asks, false)
}

// TakeMarket removes quantity from the best level on the opposite side.
// Buy market orders consume asks; sell market orders consume bids.
// Returns filled quantity (may be less than requested if the book is thin).
func (ob *OrderBook) TakeMarket(side order.Side, qty int64) (int64, error) {
	if qty <= 0 {
		return 0, fmt.Errorf("book: quantity must be positive")
	}

	var levels map[int64]*priceLevel
	var highestFirst bool
	switch side {
	case order.Buy:
		levels = ob.asks
		highestFirst = false
	case order.Sell:
		levels = ob.bids
		highestFirst = true
	default:
		return 0, fmt.Errorf("book: unknown side %v", side)
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
