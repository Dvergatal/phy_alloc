#
# Makefile for InsydeFlash tool for Linux (build your main kernel)
#
MACHINE:=$(shell uname -r)
KERNEL_DIR=/lib/modules/$(MACHINE)/build
CROSS_COMPILE=

obj-m+=phy_alloc.o
LIB_PATH += /usr/lib/insyde/driver/

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
  EXTRA_CFLAGS += -D__X86_64__
endif

ccflags-y += $(EXTRA)

all: phy_alloc

install:
	mkdir -p $(LIB_PATH)
	cp ./phy_alloc.ko $(LIB_PATH)
		
remove:
	rm -rf $(LIB_PATH)
	
phy_alloc:
	make -C ${KERNEL_DIR} M=$(PWD) CROSS_COMPILE=${CROSS_COMPILE} modules

clean:
	make -C ${KERNEL_DIR} M=$(PWD) clean

embedded:
	KCFLAGS=-D__STATIC_REGISTER make -C ${KERNEL_DIR} M=$(PWD) CROSS_COMPILE=${CROSS_COMPILE}  modules

