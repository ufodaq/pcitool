BINARIES += pci

INCDIR += 
LDINC += $(addprefix -L ,$(LIBDIR))
LDFLAGS += 

all: $(BINARIES)

.PHONY: all depend clean

include common.mk

###############################################################
# Target definitions

OBJECTS = pci.o ipecamera.o default.o tools.o

libpcilib.so: $(OBJECTS)
	echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -shared -o $@ $(OBJECTS)

pci: cli.o libpcilib.so
	echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -L. -lpcilib -o $@ $<

clean:
	@echo -e "CLEAN \t$(shell pwd)"
	-$(Q)rm -f $(addprefix $(BINDIR)/,$(BINARIES))
	-$(Q)rm -f $(OBJ)
	-$(Q)rm -f $(DEPEND)
