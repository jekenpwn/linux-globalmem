#######################################################################
# Author: Jeken
# Kernel module build file
#######################################################################


TOP=$(shell pwd)


ifdef $(KDIR)
U_INCLUDE += -I$(KDIR)/include
ccflags-y := 
endif

ifndef $(ARCH)
ARCH=arm
endif

obj-m += globalmem.o 
CFILES += globalmem.c 
LDFLAGS :=

target:
	make --no-print-directory -C $(KDIR) M=$(TOP)  modules
    
all:
	make clean
	make target
clean:
	make -C $(KDIR) M=$(TOP) clean
	
