obj-m += ir_avoid.o
KDIR = /lib/modules/$(shell uname -r)/build
all:
	make -C $(KDIR) M=$(shell pwd) modules
clean:
	make -C $(KDIR) M=$(shell pwd) clean
install:
	/bin/sh remove_xz.sh
	xz -z -0 ir_avoid.ko
	cp -rf ir_avoid.ko.xz /lib/modules/$(shell uname -r)/kernel/drivers/leds
	depmod -a
