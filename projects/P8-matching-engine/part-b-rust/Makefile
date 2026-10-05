# P8 part-b-rust — cargo 包装（与 part-a 的 make test 对齐）
CARGO ?= cargo

.PHONY: all test bench build clean

all: test

test:
	$(CARGO) test --release

build:
	$(CARGO) build --release

clean:
	$(CARGO) clean
