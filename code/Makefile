obj-m += super.o
obj-m += dor.o
dor-y := ram_block.o ram_device.o partition.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
