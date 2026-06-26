.PHONY: all build run test clean

all: test

build:
	go build -o bin/exchange .

run: build
	./bin/exchange

test:
	go test -v .

clean:
	go clean -testcache
	rm -rf bin/
