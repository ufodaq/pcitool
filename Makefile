BINARIES += pci

INCDIR += ./
LDINC += $(addprefix -L ,$(LIBDIR))
LDFLAGS += 
DESTDIR ?= /usr/local

all: $(BINARIES)

.PHONY: all depend clean

include common.mk

###############################################################
# Target definitions

OBJECTS = pci.o kmem.o dma.o  default.o tools.o dma/nwl.o ipecamera/model.o ipecamera/image.o 

libpcilib.so: $(OBJECTS)
	echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -shared -o $@ $(OBJECTS)

pci: cli.o libpcilib.so
	echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -L. -lpcilib -o $@ $<

install: pci
	install -m 644 pcilib.h $(DESTDIR)/include
	install -m 644 ipecamera/ipecamera.h $(DESTDIR)/include
	if [ -d $(DESTDIR)/lib64 ]; then install -m 755 libpcilib.so $(DESTDIR)/lib64; else install -m 755 libpcilib.so $(DESTDIR)/lib; fi
	install -m 755 pci $(DESTDIR)/bin
	ldconfig

clean:
	@echo -e "CLEAN \t$(shell pwd)"
	-$(Q)rm -f $(addprefix $(BINDIR)/,$(BINARIES))
	-$(Q)rm -f $(OBJ)
	-$(Q)rm -f $(DEPEND)
