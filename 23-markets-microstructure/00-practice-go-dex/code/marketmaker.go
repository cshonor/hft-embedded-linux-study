package main

import (
	"context"
	"fmt"
	"time"
)

const ownerMM = "mm"

// MarketMaker 极简做市：定时在买一 / 卖一附近挂小额限价单，给散户提供对手盘（Ch 2 §1）
type MarketMaker struct {
	Book       *Orderbook
	QuoteSize  float64       // 每边挂单量
	Spread     float64       // 做市 spread（ask - bid）
	RefPrice   float64       // 簿空时的参考 mid
	Interval   time.Duration // 刷新间隔
}

// Refresh 撤掉旧 MM 单，按当前 Level 1 重新挂买卖双边
func (mm *MarketMaker) Refresh(ts int64) {
	mm.Book.CancelByOwner(ownerMM)
	bid, ask := mm.quotePrices()
	mm.Book.AddOrder(&Order{
		OrderType: OrderLimit,
		Side:      Buy,
		Price:     bid,
		Size:      mm.QuoteSize,
		Timestamp: ts,
		Owner:     ownerMM,
	})
	mm.Book.AddOrder(&Order{
		OrderType: OrderLimit,
		Side:      Sell,
		Price:     ask,
		Size:      mm.QuoteSize,
		Timestamp: ts,
		Owner:     ownerMM,
	})
}

// quotePrices 目标：形成 Level 1 的 bid / ask；簿上已有散户单时 mid 跟随全簿极值
func (mm *MarketMaker) quotePrices() (bid, ask float64) {
	bb, _, bbOK := mm.Book.BestBid()
	ba, _, baOK := mm.Book.BestAsk()

	switch {
	case bbOK && baOK:
		mid := (bb + ba) / 2
		half := mm.Spread / 2
		return mid - half, mid + half
	case bbOK:
		return bb, bb + mm.Spread
	case baOK:
		return ba - mm.Spread, ba
	default:
		half := mm.Spread / 2
		return mm.RefPrice - half, mm.RefPrice + half
	}
}

// Run 后台定时刷新报价；ctx 取消时退出
func (mm *MarketMaker) Run(ctx context.Context) {
	if mm.Interval <= 0 {
		mm.Interval = 500 * time.Millisecond
	}
	ticker := time.NewTicker(mm.Interval)
	defer ticker.Stop()

	mm.Refresh(time.Now().UnixNano())
	for {
		select {
		case <-ctx.Done():
			return
		case t := <-ticker.C:
			mm.Refresh(t.UnixNano())
		}
	}
}

// PrintLevel1 打印 Level 1 BBO（全簿最优，含 MM + 散户）
func PrintLevel1(book *Orderbook, label string) {
	bid, bidSz, bidOK := book.BestBid()
	ask, askSz, askOK := book.BestAsk()
	if !bidOK && !askOK {
		fmt.Printf("[%s] Level 1: (empty)\n", label)
		return
	}
	spread, spreadOK := book.Spread()
	if spreadOK {
		fmt.Printf("[%s] Level 1: bid=%.2f x %.4f | ask=%.2f x %.4f | spread=%.2f\n",
			label, bid, bidSz, ask, askSz, spread)
		return
	}
	fmt.Printf("[%s] Level 1: bid=%.2f x %.4f | ask=%.2f x %.4f\n", label, bid, bidSz, ask, askSz)
}
