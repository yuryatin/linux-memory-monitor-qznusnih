obj-m := qznusnih.o

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	-$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
