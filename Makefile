# kbuild part of makefile
obj-m += procfs2.o
#normal makefile
	KDIR ?=/usr/include/linux/

default:
	gcc -C $(KDIR) 

