
# --------------------------------------------

Q = @

CC = $(CROSS)gcc
CXX = $(CROSS)g++
CFLAGS = -O3 -Wall
CXXFLAGS = -O3 -Wall -fno-exceptions
LD = $(CROSS)ld
STRIP = $(CROSS)strip

SHELL = /bin/sh

PWD := $(shell pwd)

# --------------------------------------------

.SUFFIXES:
.SUFFIXES: .cpp .c .o .java .class .jar

%.o: %.cpp
	@$(if $(Q), echo "  C++    " $@ )
	$(Q)$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o: %.c
	@$(if $(Q), echo "  CC     " $@ )
	$(Q)$(CC) $(CFLAGS) -o $@ -c $< 

%: %.o
	@$(if $(Q), echo "  LD     " $@ )
	$(Q)$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
ifdef STRIP_EXE
	@$(if R(Q), echo "  STRIP  " $@ )
	$(Q)$(STRIP) -s --remove-section=.note --remove-section=.comment $@
endif

%.ko: %.c
ifndef KERNELDIR
	$(error "KERNELDIR must be specified!")
endif
	$(Q)$(MAKE) -C $(KERNELDIR) M=$(PWD) modules --no-print-directory

%.class: %.java
	@$(if $(Q), echo "  JAVAC  " $@ )
	$(Q)javac $<

%.jar:
	@$(if $(Q), echo "  JAR    " $@ )
	$(Q)jar cf $@ $^

# --------------------------------------------

ifndef CROSS

.PHONY: all

all: $(build_list)

else

.PHONY: all

all: report_cross_compiling $(build_list)

.PHONY: report_cross_compiling

report_cross_compiling:;
	$(if $(Q), @echo "  Note   " CROSS=$(CROSS) )

endif

# --------------------------------------------

.PHONY: install

ifdef INSTALLDIR

ifdef Q

define INST_template
	@echo "  COPY   " $(1) "->" $(INSTALLDIR)
	$(Q)cp -a $(1) $(INSTALLDIR)

endef

else # !Q

define INST_template
	cp -a $(1) $(INSTALLDIR)

endef

endif # Q

else # ! INSTALLDIR

define INST_template
	@true

endef

endif # INSTALLDIR
	
install: all
	$(foreach prog, $(build_list), \
	    $(call INST_template, $(prog) ) )

# --------------------------------------------

.PHONY: clean

clean: FORCE
	@$(if $(Q), echo "  RM     " $(clean_list) )
	$(Q)rm -rf $(clean_list) 2>/dev/null ; true
ifdef clean_list2
	@$(if $(Q), echo "  RM     " $(clean_list2) )
	$(Q)rm -rf $(clean_list2) 2>/dev/null ; true
endif
ifdef clean_list3
	@$(if $(Q), echo "  RM     " $(clean_list3) )
	$(Q)rm -rf $(clean_list3) 2>/dev/null ; true
endif

# --------------------------------------------

FORCE:

# --------------------------------------------
