CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
.PHONY: all clean
all:
	$(MAKE) -C demo_argv
	$(MAKE) -C demo_argv_addrs
	$(MAKE) -C demo_argv_sizeof
	$(MAKE) -C demo_param_decay
	$(MAKE) -C demo_star_binds_name
	$(MAKE) -C demo_argv_usage
	$(MAKE) -C demo_argv_mutate
	$(MAKE) -C demo_ptr_arr_vs_arr_ptr
clean:
	$(MAKE) -C demo_argv clean
	$(MAKE) -C demo_argv_addrs clean
	$(MAKE) -C demo_argv_sizeof clean
	$(MAKE) -C demo_param_decay clean
	$(MAKE) -C demo_star_binds_name clean
	$(MAKE) -C demo_argv_usage clean
	$(MAKE) -C demo_argv_mutate clean
	$(MAKE) -C demo_ptr_arr_vs_arr_ptr clean
