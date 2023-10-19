KDIR=/local/patrickfrank.tchossiewedjengoue.etu/build/kvm/

obj-m += asee_mod.o
PWD := $(CURDIR)

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
