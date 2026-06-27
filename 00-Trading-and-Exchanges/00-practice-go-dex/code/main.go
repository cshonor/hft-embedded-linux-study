package main

import (
	"context"
	"fmt"
	"time"
)

func main() {
	book := &Orderbook{}
	mm := &MarketMaker{
		Book:      book,
		QuoteSize: 0.01,
		Spread:    2.0,
		RefPrice:  50000,
		Interval:  500 * time.Millisecond,
	}

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	go mm.Run(ctx)

	time.Sleep(600 * time.Millisecond)
	PrintLevel1(book, "MM seeded")

	// 散户市价买入 → 吃 MM 卖一
	trades := book.AddOrder(&Order{
		OrderType: OrderMarket,
		Side:      Buy,
		Size:      0.005,
		Timestamp: time.Now().UnixNano(),
		Owner:     "retail",
	})
	for i, tr := range trades {
		fmt.Printf("  trade #%d: buy %.4f @ %.2f (maker=%s)\n", i+1, tr.Size, tr.Price, tr.Maker.Owner)
	}
	PrintLevel1(book, "after retail buy")

	time.Sleep(600 * time.Millisecond)
	PrintLevel1(book, "MM refreshed")

	// 散户市价卖出 → 吃 MM 买一
	trades = book.AddOrder(&Order{
		OrderType: OrderMarket,
		Side:      Sell,
		Size:      0.003,
		Timestamp: time.Now().UnixNano(),
		Owner:     "retail",
	})
	for i, tr := range trades {
		fmt.Printf("  trade #%d: sell %.4f @ %.2f (maker=%s)\n", i+1, tr.Size, tr.Price, tr.Maker.Owner)
	}
	PrintLevel1(book, "after retail sell")
}
