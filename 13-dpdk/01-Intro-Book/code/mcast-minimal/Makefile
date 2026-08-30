# SPDX-License-Identifier: BSD-3-Clause
#
# mcast-minimal —— UDP 组播收包最小工程
#
#   mcast_socket_ref  内核协议栈对照版，无依赖，任何 Linux 都能编
#   mcast_minimal     DPDK 旁路版，需要 DPDK 22.11 LTS / 23.11 LTS
#
# 没有装 DPDK 的机器上 make 会自动跳过 DPDK 版而不是报错，
# 这样对照组（baseline）永远能先跑起来拿到数据。

RTE_SDK ?=
PKGCONF ?= pkg-config

TARGET_DPDK = mcast_minimal
TARGET_REF  = mcast_socket_ref

CC       ?= gcc
DPDK_CFLAGS := -O3 -march=native -Wall -Wextra
REF_CFLAGS  := -O2 -Wall -Wextra

DPDK_AVAIL := $(shell $(PKGCONF) --exists libdpdk 2>/dev/null && echo yes)

ifeq ($(DPDK_AVAIL),yes)
DPDK_CFLAGS += $(shell $(PKGCONF) --cflags libdpdk)
DPDK_LDLIBS := $(shell $(PKGCONF) --libs libdpdk)
ALL_TARGETS := $(TARGET_REF) $(TARGET_DPDK)
else
ALL_TARGETS := $(TARGET_REF)
endif

all: $(ALL_TARGETS)
ifeq ($(DPDK_AVAIL),yes)
	@echo "---"
	@echo "两个目标均已构建。先用对照组跑出 baseline，再跑 DPDK 版做对比。"
else
	@echo "---"
	@echo "未检测到 libdpdk，只构建了对照组 $(TARGET_REF)。"
	@echo "只构建它不需要 DPDK / 大页 / 网卡绑定，任何 Linux 都能跑。"
	@echo "装好 DPDK 后重新 make 即可获得 $(TARGET_DPDK)。"
endif

$(TARGET_DPDK): src/main.c src/hist.h
	$(CC) $(DPDK_CFLAGS) -o $@ src/main.c $(DPDK_LDLIBS)

$(TARGET_REF): src/mcast_socket_ref.c src/hist.h
	$(CC) $(REF_CFLAGS) -o $@ src/mcast_socket_ref.c

clean:
	rm -f $(TARGET_DPDK) $(TARGET_REF)

.PHONY: all clean
