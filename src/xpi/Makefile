
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
# Otherwise we were called directly from the command
# line; invoke the kernel build system.

%.ko: %.c
	$(MAKE) -C /pub/sysapps/src/linux-2.6.19 M=$(PWD) modules --no-print-directory

ifneq ($(KERNELRELEASE),)

obj-m := xpi.o

else

build_list = xpi.ko
clean_list = $(build_list)
clean_list2 = xpi*.o xpi.mod.* .tmp_versions
clean_list3 = *.o .xpi*cmd 

xpi.ko: xpi.c xpi.h

endif



