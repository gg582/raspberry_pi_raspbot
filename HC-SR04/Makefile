obj-m += sr04.o
KDIR = /lib/modules/$(shell uname -r)/build
all:
	make -C $(KDIR) M=$(shell pwd) modules
clean:
	make -C $(KDIR) M=$(shell pwd) clean
install:
	/bin/sh remove_xz.sh
	xz -z -0 sr04.ko
	cp -f sr04.ko.xz /lib/modules/$(shell uname -r)/kernel/drivers/leds
	depmod -a
