
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
# Otherwise we were called directly from the command
# line; invoke the kernel build system.

ifneq ($(KERNELRELEASE),)

obj-m := hpi.o

else

build_list = hpi.ko
clean_list = $(build_list)
clean_list2 = hpi*.o hpi.mod.* .tmp_versions
clean_list3 = *.o .hpi*cmd 

include ../Makefile.inc

hpi.ko: hpi.c hpi.h

endif



