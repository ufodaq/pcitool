BINARIES += pci

INCDIR += 
LDINC += $(addprefix -L ,$(LIBDIR))
LDFLAGS += 

all: $(BINARIES)

.PHONY: all depend clean

include common.mk


###############################################################
# Target definitions


pci: pci.o tools.o
	echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< tools.o

clean:
	@echo -e "CLEAN \t$(shell pwd)"
	-$(Q)rm -f $(addprefix $(BINDIR)/,$(BINARIES))
	-$(Q)rm -f $(OBJ)
	-$(Q)rm -f $(DEPEND)
