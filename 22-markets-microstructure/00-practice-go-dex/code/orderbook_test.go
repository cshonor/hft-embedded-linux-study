package main

import "testing"

func TestMatchLimitCross(t *testing.T) {
	book := &Orderbook{}
	book.AddOrder(&Order{
		OrderType: OrderLimit, Side: Sell, Price: 100, Size: 1,
		Timestamp: 1, Owner: "retail",
	})
	trades := book.AddOrder(&Order{
		OrderType: OrderLimit, Side: Buy, Price: 100, Size: 0.5,
		Timestamp: 2, Owner: "retail",
	})
	if len(trades) != 1 || trades[0].Size != 0.5 || trades[0].Price != 100 {
		t.Fatalf("unexpected trades: %+v", trades)
	}
	if book.Asks[0].TotalVolume != 0.5 {
		t.Fatalf("ask remainder: %.4f", book.Asks[0].TotalVolume)
	}
}

func TestMarketBuyWalkBook(t *testing.T) {
	book := &Orderbook{}
	book.AddOrder(&Order{OrderType: OrderLimit, Side: Sell, Price: 101, Size: 1, Timestamp: 1, Owner: "retail"})
	book.AddOrder(&Order{OrderType: OrderLimit, Side: Sell, Price: 102, Size: 1, Timestamp: 2, Owner: "retail"})
	trades := book.AddOrder(&Order{OrderType: OrderMarket, Side: Buy, Size: 1.5, Timestamp: 3, Owner: "retail"})
	if len(trades) != 2 || trades[0].Price != 101 || trades[1].Price != 102 {
		t.Fatalf("walk book: %+v", trades)
	}
}

func TestBestBidAskLevel1(t *testing.T) {
	book := &Orderbook{}
	book.AddOrder(&Order{OrderType: OrderLimit, Side: Buy, Price: 99, Size: 1, Timestamp: 1, Owner: "retail"})
	book.AddOrder(&Order{OrderType: OrderLimit, Side: Buy, Price: 100, Size: 2, Timestamp: 2, Owner: "retail"})
	book.AddOrder(&Order{OrderType: OrderLimit, Side: Sell, Price: 101, Size: 1, Timestamp: 3, Owner: "retail"})
	book.AddOrder(&Order{OrderType: OrderLimit, Side: Sell, Price: 102, Size: 1, Timestamp: 4, Owner: "retail"})

	bid, _, ok := book.BestBid()
	if !ok || bid != 100 {
		t.Fatalf("best bid = %v ok=%v", bid, ok)
	}
	ask, _, ok := book.BestAsk()
	if !ok || ask != 101 {
		t.Fatalf("best ask = %v ok=%v", ask, ok)
	}
	sp, ok := book.Spread()
	if !ok || sp != 1 {
		t.Fatalf("spread = %v", sp)
	}
}

func TestMarketMakerProvidesBBO(t *testing.T) {
	book := &Orderbook{}
	mm := &MarketMaker{Book: book, QuoteSize: 0.1, Spread: 10, RefPrice: 50000}
	mm.Refresh(1)

	bid, _, bidOK := book.BestBid()
	ask, _, askOK := book.BestAsk()
	if !bidOK || !askOK {
		t.Fatal("MM should seed both sides")
	}
	if ask-bid != 10 {
		t.Fatalf("spread = %.2f want 10", ask-bid)
	}

	trades := book.AddOrder(&Order{
		OrderType: OrderMarket, Side: Buy, Size: 0.05, Timestamp: 2, Owner: "retail",
	})
	if len(trades) != 1 || trades[0].Maker.Owner != ownerMM {
		t.Fatalf("retail should hit MM ask: %+v", trades)
	}
}

func TestMarketMakerCancelRefresh(t *testing.T) {
	book := &Orderbook{}
	mm := &MarketMaker{Book: book, QuoteSize: 0.1, Spread: 10, RefPrice: 50000}
	mm.Refresh(1)
	mm.Refresh(2)

	count := 0
	for _, lim := range book.Asks {
		for _, o := range lim.Orders {
			if o.Owner == ownerMM {
				count++
			}
		}
	}
	for _, lim := range book.Bids {
		for _, o := range lim.Orders {
			if o.Owner == ownerMM {
				count++
			}
		}
	}
	if count != 2 {
		t.Fatalf("want 2 MM orders after refresh, got %d", count)
	}
}
