# Host-side tests that should pass on WSL (g++/gcc + make).
# P4 .ko / P5 板子 / P3.5 QEMU / P7 真 DPDK 不在这里跑。

.PHONY: test
test:
	$(MAKE) -C P1-cpu-simulator/part-a-alu-host test
	$(MAKE) -C P1-cpu-simulator/part-b-multicycle test
	$(MAKE) -C P2-shell-malloc/part-a-shell test
	$(MAKE) -C P2-shell-malloc/part-b-malloc test
	$(MAKE) -C P2-shell-malloc/part-c-exercises test
	$(MAKE) -C P2.5-c-toolkit/part-a-toolkit test
	$(MAKE) -C P3-http-server/part-a-c-server test
	$(MAKE) -C P3-http-server/part-b-cpp-rewrite test
	$(MAKE) -C P4-kernel-module/part-a-hello-chardev test
	$(MAKE) -C P6-network-protocol-analyzer/part-a-parser test
	$(MAKE) -C P7-dpdk-forwarder-profiling/part-a-host-poll test
	$(MAKE) -C P8-matching-engine/part-a-lob test
	$(MAKE) -C P10-hft-prototype/part-a-demo test
	@echo "projects host tests: OK"
