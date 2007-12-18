build_list = ipSetup
clean_list = *.o $(build_list)

include ../Makefile.inc

# --------------------------

ifndef CROSS
    CXXFLAGS += -DCONFIG_READLINE
    LDFLAGS += -lreadline -lncurses
else
    CXXFLAGS += -DBUSYBOX
endif

ipSetup : ipSetup.o

ipSetup.o : ipSetup.cpp

