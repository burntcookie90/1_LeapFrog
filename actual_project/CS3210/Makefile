ifneq ($(KERNELRELEASE),) 
   obj-m := lfprng12.o
else 

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 

PWD := $(shell pwd)

default: 
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm Test

test:
	gcc -fopenmp -Wall test.c -o Test
