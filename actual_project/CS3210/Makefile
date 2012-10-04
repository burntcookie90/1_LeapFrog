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
	rm Test 1_LeapFrog_Team_HAM.tar.gz

test:
	gcc -fopenmp -Wall test.c -o Test

reftest:
	gcc -fopenmp Reference+Test+Program.c -o RefTest

submit:
	tar cvf 1_LeapFrog_Team_HAM.tar test.c lfprng12.c paper/WriteUp.pdf Makefile README.md Reference+Test+Program.c
	gzip 1_LeapFrog_Team_HAM.tar
