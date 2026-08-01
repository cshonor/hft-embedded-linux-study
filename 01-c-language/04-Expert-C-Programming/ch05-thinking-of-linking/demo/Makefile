CC      ?= gcc

.PHONY: all clean

all:
	$(MAKE) -C demo01_four_stage
	$(MAKE) -C demo02_static_lib
	$(MAKE) -C demo03_weak_strong
	$(MAKE) -C demo04_linker_script

clean:
	$(MAKE) -C demo01_four_stage clean
	$(MAKE) -C demo02_static_lib clean
	$(MAKE) -C demo03_weak_strong clean
	$(MAKE) -C demo04_linker_script clean
