
obj-m := pciDriver.o
pciDriver-objs := base.o int.o umem.o kmem.o sysfs.o ioctl.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
INSTALLDIR ?= /lib/modules/$(shell uname -r)/extra
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	@mkdir -p $(INSTALLDIR)
	@echo "INSTALL $(INSTALLDIR)/pciDriver.ko"
	@install -m 755 pciDriver.ko $(INSTALLDIR)
	@echo "INSTALL /usr/include/pciDriver/driver/pciDriver.h"
	@mkdir -p /usr/include/pciDriver/driver
	@install -m 644 pciDriver.h /usr/include/pciDriver/driver

uninstall:
	@echo "UNINSTALL $(INSTALLDIR)/pciDriver.ko"
	@rm -f $(INSTALLDIR)/pciDriver.ko
	@echo "UNINSTALL /usr/include/pciDriver/driver/pciDriver.h"
	@rm -rf /usr/include/pciDriver/driver

clean:
	rm -rf *.o *.ko *.mod.c .*.o.cmd .*.o.tmp .*.ko.cmd  .*.o *.symvers modules.order .tmp_versions
