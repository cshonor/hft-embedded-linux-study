package main

import "testing"

func TestBestBidAskAfterLimitInsert(t *testing.T) {
	ob := NewOrderBook()

	orders := []*Order{
		{ID: 1, Side: Buy, Type: Limit, Price: 100, Qty: 10, Time: 1},
		{ID: 2, Side: Buy, Type: Limit, Price: 101, Qty: 5, Time: 2},
		{ID: 3, Side: Sell, Type: Limit, Price: 103, Qty: 8, Time: 3},
		{ID: 4, Side: Sell, Type: Limit, Price: 102, Qty: 4, Time: 4},
	}
	for _, o := range orders {
		if err := ob.AddLimit(o); err != nil {
			t.Fatalf("AddLimit(%d): %v", o.ID, err)
		}
	}

	bid, ok := ob.BestBid()
	if !ok || bid != 101 {
		t.Fatalf("BestBid = (%d, %v), want (101, true)", bid, ok)
	}
	ask, ok := ob.BestAsk()
	if !ok || ask != 102 {
		t.Fatalf("BestAsk = (%d, %v), want (102, true)", ask, ok)
	}
}

func TestMarketOrderEatsBestLevel(t *testing.T) {
	ob := NewOrderBook()

	for _, o := range []*Order{
		{ID: 1, Side: Sell, Type: Limit, Price: 100, Qty: 3, Time: 1},
		{ID: 2, Side: Sell, Type: Limit, Price: 101, Qty: 7, Time: 2},
	} {
		if err := ob.AddLimit(o); err != nil {
			t.Fatalf("AddLimit: %v", err)
		}
	}

	filled, err := ob.TakeMarket(Buy, 5)
	if err != nil {
		t.Fatalf("TakeMarket: %v", err)
	}
	if filled != 5 {
		t.Fatalf("filled = %d, want 5", filled)
	}

	ask, ok := ob.BestAsk()
	if !ok || ask != 101 {
		t.Fatalf("BestAsk after partial take = (%d, %v), want (101, true)", ask, ok)
	}

	filled, err = ob.TakeMarket(Buy, 10)
	if err != nil {
		t.Fatalf("TakeMarket second: %v", err)
	}
	if filled != 5 {
		t.Fatalf("filled = %d, want 5", filled)
	}
	if _, ok := ob.BestAsk(); ok {
		t.Fatal("expected empty ask side")
	}
}
