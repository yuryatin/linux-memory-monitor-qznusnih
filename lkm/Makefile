TARGET := lmm_qznusnih
obj-m := $(TARGET).o
$(TARGET)-objs := src/main.o

KDIR := /home/alex/poky/build/tmp/work/qemux86-poky-linux/linux-yocto/6.12.38+git/linux-yocto-6.12.38+git
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
