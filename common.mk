# Compiler and default flags
CC ?= gcc
CFLAGS ?= -O2


# Defaults for directories
ROOTDIR ?= $(shell pwd)

INCDIR ?= $(ROOTDIR)
BINDIR ?= $(ROOTDIR)
LIBDIR ?= $(ROOTDIR)
OBJDIR ?= $(ROOTDIR)
DEPENDDIR ?= $(ROOTDIR)

CXXFLAGS += $(addprefix -I ,$(INCDIR))
CFLAGS += $(addprefix -I ,$(INCDIR))

# Source files in this directory
SRC = $(wildcard *.cpp)
SRCC = $(wildcard *.c)

# Corresponding object files 
OBJ = $(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))
OBJ += $(addprefix $(OBJDIR)/,$(SRCC:.c=.o))

# Corresponding dependency files
DEPEND = $(addprefix $(DEPENDDIR)/,$(SRC:.cpp=.d)) 
DEPEND += $(addprefix $(DEPENDDIR)/,$(SRCC:.c=.d)) 

# This makes Verbose easier. Just prefix $(Q) to any command
ifdef VERBOSE
	Q ?= 
else
	Q ?= @
endif

###############################################################
# Target definitions

# Target for automatic dependency generation
depend: $(DEPEND) $(DEPENDC);

# This rule generates a dependency makefile for each source
$(DEPENDDIR)/%.d: %.c
	@echo -e "DEPEND \t$<"
	$(Q)$(CC) $(addprefix -I ,$(INCDIR)) -MM -MF $@ \
		-MT $(OBJDIR)/$(<:.c=.o) -MT $@ $< 

# This includes the automatically 
# generated dependency files
-include $(DEPEND)

$(OBJDIR)/%.o: %.c
	@echo -e "CC \t$<"
	$(Q)@$(CC) $(CFLAGS) -c -o $@ $<
