CFILES := main.c device.c
obj-m := memo.o 
memo-objs := $(CFILES:.c=.o)

KBUILD_DIR := /lib/modules/`uname -r`/build

ccflags-y += -Wno-declaration-after-statement

all:
	$(MAKE) -C $(KBUILD_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KBUILD_DIR) M=$(PWD) clean
