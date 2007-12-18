
build_list = c54load
clean_list = *.o $(build_list)

include ../Makefile.inc

LDFLAGS = -lpthread

# --------------------------

c54load : c54load.o hpiwrapper.o main.o

main.o : main.cpp c54load.h hpiwrapper.h

c54load.o : c54load.cpp c54load.h

hpiwrapper.o : hpiwrapper.cpp hpiwrapper.h ../hpi/hpi.h

