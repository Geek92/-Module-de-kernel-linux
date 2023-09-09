KDIR=../../build/kvm/

obj-m += hello-1.o 
PWD := $(CURDIR) 

all: 
	make -C $(KDIR) M=$(PWD) modules 
clean: 
	make -C $(KDIR) M=$(PWD) clean
