CONFIG_MODULE_SIG=n

obj-m := pciDriver.o
pciDriver-objs := base.o dev.o int.o umem.o kmem.o sysfs.o ioctl.o pcibus.o rdma.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
INSTALLDIR ?= /lib/modules/$(shell uname -r)/extra
MAININSTALLDIR ?= /lib/modules/$(shell uname -r)/kernel/extra
HEADERDIR ?= /lib/modules/$(shell uname -r)/source/include
PWD := $(shell pwd)

EXTRA_CFLAGS += -I$(M)/..

ifdef DUMMY_DEVICE
ifneq ($(DUMMY_DEVICE), 0)
    EXTRA_CFLAGS += -DPCIDRIVER_DUMMY_DEVICE
endif
endif

default:
	@KERNEL_GCC_VERSION=`cat /proc/version | head -n1 | cut -d " " -f 7` ;\
	GCC_VERSION=`$(CC) --version | head -n 1 | tr ' ' '\n' | grep -e "[0-9]\+\.[0-9]" | tail -n 1` ;\
	if [ $$KERNEL_GCC_VERSION != $$GCC_VERSION ]; then \
	    echo "Kernel is compiled with gcc $$KERNEL_GCC_VERSION, but you are now using $$GCC_VERSION" ;\
	    GCC_MAJOR=`echo $$KERNEL_GCC_VERSION | cut -d "." -f 1-2` ;\
	    newCC=gcc-$$GCC_MAJOR ;\
	    CC=`which $$newCC 2>/dev/null` ;\
	    if [ $$? -ne 0 ]; then \
		echo "No compiler of $$GCC_MAJOR series is installed" ;\
		exit 1 ;\
	    fi ;\
	    GCC_VERSION=`$$CC --version | head -n 1 | tr ' ' '\n' | grep -e "[0-9]\+\.[0-9]" | tail -n 1` ;\
	    if [ $$KERNEL_GCC_VERSION != $$GCC_VERSION -a -z "$$RELAXED_GCC_CHECK" ]; then \
		echo "The $$GCC_VERSION of $$GCC_MAJOR series is installed" ;\
		exit 1 ;\
	    fi ;\
	    echo "Setting CC to $$newCC" ;\
	else \
	    CC=$(CC) ;\
	fi ;\
	build_revision=`bzr revno` ;\
	build_author=`bzr log -r$$build_revision | grep committer | cut -c 12-` ;\
	build_path=`pwd` ;\
	build_branch=`bzr nick` ;\
	build_lastmod=`find . -type f -name '*.[ch]' -printf '%TY/%Tm/%Td %TH:%TM:%TS  %p\n' | grep -E -v 'build.h|mod.c' | sort -n  | tail -n 1 | cut -d ' ' -f 1-2 | cut -d '.' -f 1` ;\
	build_changes=`bzr status -SV | cut -c 5- | tr \\\n " "` ;\
	build_user=`whoami` ;\
	build_date=`date "+%Y/%m/%d %H:%M:%S"` ;\
	build="Built at $$build_date by $$build_user" ;\
	revision="Revision $$build_revision from $$build_branch by $$build_author at $$build_path, last modification from $$build_lastmod" ;\
	/bin/echo -e "#define PCIDRIVER_BUILD \"$$build\"\\n#define PCIDRIVER_REVISION \"$$revision\"\\n#define PCIDRIVER_CHANGES \"$$build_changes\"\\n" > build.h ;\
	$(MAKE) $(CFLAGS) -C $(KERNELDIR) M=$(PWD) CC=$$CC modules

install:
	@mkdir -p $(INSTALLDIR)
	@echo "INSTALL $(INSTALLDIR)/pciDriver.ko"
	@install -m 755 pciDriver.ko $(INSTALLDIR)
	@echo "INSTALL $(INSTALLDIR)/pciDriver.symvers"
	@install -m 644 Module.symvers $(INSTALLDIR)/pciDriver.symvers
	@echo "INSTALL $(HEADERDIR)/linux/pcidriver.h"
	@install -m 644 pcidriver.h $(HEADERDIR)/linux/
	@echo "INSTALL /usr/include/linux/pcidriver.h"
	@mkdir -p /usr/include/linux
	@install -m 644 ioctl.h /usr/include/linux/pcidriver.h

uninstall:
	@echo "UNINSTALL $(INSTALLDIR)/pciDriver.ko"
	@rm -f $(INSTALLDIR)/pciDriver.ko
	@rm -f $(MAININSTALLDIR)/pciDriver.ko
	@rm -f $(INSTALLDIR)/pciDriver.symvers
	@rm -f $(MAININSTALLDIR)/pciDriver.symvers
	@echo "UNINSTALL /usr/include/linux/pcidriver.h"
	@rm -rf /usr/include/pciDriver/
	@rm -rf /usr/include/linux/pcidriver.h
	@echo "UNINSTALL $(HEADERDIR)/linux/pcidriver.h"
	@rm -rf $(HEADERDIR)/linux/pcidriver.h

clean:
	rm -rf *.o *.ko *.mod.c .*.o.cmd .*.o.tmp .*.ko.cmd  .*.o *.symvers modules.order .tmp_versions
